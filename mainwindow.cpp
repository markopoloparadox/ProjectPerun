#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gamelibrary.h"
#include <fstream>
#include <cstring>
#include <QString>
#include <QByteArray>

#if defined (_WIN32)
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#endif

#if defined (__linux__)
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <cmath>

/*char* add_zeros_before (short value, short numOfDigits = 2) {
    char result[5] = "";
    short i;
    for (i = 0 ; i<numOfDigits ; i++) {
        result[i] = 48+value/pow(10,numOfDigits-i-1);
        value = value%pow(10,numOfDigits-i-1);
    }
    result[i] = '\0';
    return result;
}*/
#endif

MainWindow::MainWindow(QTcpSocket *socket, qint16 port, bool aMode, QString name, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){

    ui->setupUi(this);

    this->setWindowTitle("Perun (Logged in as " + name + ")");
    this->m_Port = port;
    this->m_Socket = socket;
    this->adminMode = aMode;
    this->m_Name = name;

    m_UDPSocket = new QUdpSocket();
    m_UDPSocket->bind(this->m_Port);
    qDebug() << m_Port;
    connect(m_UDPSocket, SIGNAL(readyRead()), this, SLOT(CheckForMsg()));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(refresh_friends_list()));
    timer->start(1000);

    ui->currentStatusCBox->addItem("Online");
    ui->currentStatusCBox->addItem("Away From Keyboard");
    ui->currentStatusCBox->addItem("Busy");
    ui->currentStatusCBox->setCurrentText( this->custom_status = QString::fromUtf8("Online") );
    ui->currentlyPlayingTBox->setText( this->current_game = "" );
    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
    ui->currentlyPlayingTBox->setFocus();

    std::fstream file;
    file.open("gamepath.dat",std::ios::app); //create file if it does not exists; otherwise do nothing (it won't be changed)
    file.close();
    if ( !this->update_supported_games_list() ) {          //checks if there's any newer version of gameslist.dat (if there is, server sends it to us)
        QMessageBox msgBox;
        msgBox.setText("Error has been occurred while receiving packet with most recent list of supported games! Try again manually by clicking \"Check for Update\" button in \"Configure game library\" menu.");
        msgBox.exec();
    }
    this->refresh_games_list();     //refreshes table in "My Games" tab with games that user owns (for which (s)he defined path)

    listener_thread = new std::thread (outer_function,this);

}

MainWindow::~MainWindow(){

    listener_thread->join();
    delete listener_thread;
    m_Socket->disconnectFromHost();
    m_Socket->close();
    m_Socket->abort();
    delete m_Socket;
    m_UDPSocket->disconnectFromHost();
    m_UDPSocket->close();
    m_UDPSocket->abort();
    delete m_UDPSocket;
    delete ui;
}

void MainWindow::on_AddFriendButton_clicked(){
    AddFriend* addfrindbox = new AddFriend(m_Socket);
    addfrindbox->show();

}

void MainWindow::enter_in_critical_section(int tID1, int tID2) {    //part of Dekker's algorithm (solution for accomplishing mutual exclusion with 2 threads) - this function ensures that just one thread is at the moment doing some sensitive action (i.e. sending/receiving messages over network sockets, writing content in file)
    flags[tID1]=1;
    while (flags[tID2]!=0) {
        if (right==tID2) {
            flags[tID1]=0;
            while (right==tID2) {
                ;
                }
            flags[tID1]=1;
            }
        }
    }

void MainWindow::exit_from_critical_section(int tID1, int tID2) {     //part of Dekker's algorithm (solution for accomplishing mutual exclusion with 2 threads) - this function releases resources that were locked by tID1 thread and now it becomes available for another thread
    right=tID2;
    flags[tID1]=0;
    }

void MainWindow::refresh_friends_list(){
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;


    object["connection"] = "0011";
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->flush();
    m_Socket->waitForReadyRead();


    packet = m_Socket->readAll();
    document = QJsonDocument::fromJson(packet.constData());
    object = document.object();

    QJsonValue value = object.value("friends");
    QJsonArray array = value.toArray();

    while(ui->tableWidget->rowCount() < array.count())
        ui->tableWidget->insertRow( ui->tableWidget->rowCount());

    for(int i = 0; i < array.count(); ++i) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(array[i].toObject().value("email").toString()));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(array[i].toObject().value("custom_status").toString()));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(array[i].toObject().value("current_game").toString()));
    }
}

/*void MainWindow::on_ChatButton_clicked(){
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    if(ui->UserNamChatLineEdit->text() == m_Name) {
        ui->ChatStatusLabel->setText("Get a friend mate!");
        ui->ChatStatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    object["connection"] = "0021";
    object["username"] = ui->UserNamChatLineEdit->text();
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(1000);
    if(m_Socket->waitForReadyRead(3000)) {
        packet = m_Socket->readAll();
        document = QJsonDocument::fromJson(packet.constData());
        object = document.object();

        if(object["connection"] == "0015") {
            ui->ChatStatusLabel->setText("Nope!");
            ui->ChatStatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        }
    }
}*/

void MainWindow::CheckForMsg(){
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    packet.resize(m_UDPSocket->pendingDatagramSize());
    m_UDPSocket->readDatagram(packet.data(), packet.size());

    document = QJsonDocument::fromJson(packet.constData());
    object = document.object();

    for(auto i : m_Chatvec) {
        if(i->m_ChatId == object["chatid"].toInt()) {
            i->Update(object);
            return;
        }
    }

    m_Chatvec.push_back(new ChatBox(object, m_Socket, m_Name, this));
    m_Chatvec.at(m_Chatvec.size() - 1)->show();
}

void MainWindow::on_actionConfigure_game_library_triggered()
{
    gamelibrary* gameLibWin = new gamelibrary(this);
    gameLibWin->show();
}

void outer_function (void *arg) {    //my compiler does not not allow that threaded function is class member (i.e. method), so this is workaround - people say that that it compiler's bug
    MainWindow *mainClass = static_cast<MainWindow*>(arg);
    mainClass->check_game_status();
}

void MainWindow::check_game_status()
{
    char *running_game, *gameserver_info;
    while (true) {
        gameserver_info = NULL;//= new char [22];
        running_game = game_running_in_background();
        std::fstream file;
        if (running_game != NULL) {
            if (this->adminMode == true) {
                //gameserver_info = NULL;
                file.open("lock.dat",std::ios::app);    //file is now locked - that means that network tracing will now start because game was detected
            }
            //gameserver_info = found_gameserver_address(running_game);
        }
        send_notification_message(1,NULL,running_game,gameserver_info);
        sleep(10);
        while (running_game!=NULL && strcmp(game_running_in_background(running_game),running_game)==0) {
            qDebug() << "igra radi";
            char* tmp = NULL;   //if user isn't running application as administrator, then there is no need to execute function for seeking relevant IP address because current network traffic isn't in progress - so we are setting that (s)he isn't playing on any gameserver
            if (this->adminMode == true) {
                tmp = found_gameserver_address(running_game);
            }

            if (tmp == NULL && gameserver_info != NULL) {   //user was till now on some game server and now he is not
                delete [] gameserver_info;
                gameserver_info = NULL;
            }
            else if (tmp != NULL && gameserver_info == NULL) {  //user was till now in game, but wasn't on any game server
                gameserver_info = tmp;
                tmp = NULL;
            }
            else if (tmp == NULL && gameserver_info == NULL) {  //user still hasn't joined any game server
                sleep(10);
                continue;   //state hasn't changed - therefore we must execute other commands in this "while" block (other users already know on which game and on which gameserver are we playing)
            }
            else if (strcmp(tmp,gameserver_info)!=0) {  //user has stayed playing same game, but he switched to another game server
                delete [] gameserver_info;
                gameserver_info = tmp;
                tmp = NULL;
            }
            else if (strcmp(tmp,gameserver_info)==0) {  //if user plays on same server on which he played before 10 seconds
                delete [] tmp;
                sleep(10);
                continue;   //there is no need to send notification message if state hasn't changed
            }
            send_notification_message(1,NULL,running_game,gameserver_info); //notifies server and other users that you have changed your gaming status
            if (tmp != NULL) {
                delete [] tmp;
            }
            sleep(10);
        }
        if (running_game!=NULL) {       //the game is obviously not active anymore (otherwise program would be still in upper loop)
            delete [] running_game;            
            if (this->adminMode == true) {      //if user has not run application as administrator, then no network traffic is captured and file wasn't locked, so there is no need to unlock it
                if (gameserver_info!=NULL) {        //if we were on some gameserver at the moment when we left that game
                    delete [] gameserver_info;
                }
                file.close();       //we are unlocking the lock file because no game is active, so no game is generating network traffic (what means no need for tracking)
            }
            send_notification_message(1,NULL,NULL,NULL);    //this is required to add if user instantly starts another game after he lefts previous one
        }
    }
}

void MainWindow::send_notification_message (int tID, const char* custom_status=NULL, char* played_game_name=NULL, char* gameserver_info=NULL) {  //notifies friends that you have started playing some game (or you stopped playing) XOR you have changed your custom status (note on exlusive or because function is separately called when user changed custom status and when his/her game status was changed
    if (custom_status==NULL && played_game_name==NULL && gameserver_info==NULL) {   //if we haven't changed custom status and are not playing on any game
        if (this->current_game == "") {     //if we haven't also played anything 10 seconds before - we don't need to notify others that we are not playing anything because they already know that
            return;
        }
    }

    if (custom_status==NULL) {              //if user's GAME status was changed
        if (played_game_name==NULL) {           //if user stoped playing current game
            this->current_game = "";
        }
        else {      //if user started playing game or remained playing current game on another game server
            std::fstream file;
            tGames gameRecord;
            file.open("gameslist.dat",std::ios::in | std::ios::binary);
            int position = binarySearchWrapper(file,played_game_name);
            file.seekg(position*sizeof(tGames));
            file.read( (char*)&gameRecord,sizeof(tGames) );
            file.close();
            if (gameserver_info==NULL) {        //if user started game but isn't on any server or (s)he disconnected from current and isn't playing on any right now
                this->current_game = QString::fromUtf8(gameRecord.fullName);
            }
            else if (gameserver_info!=NULL) {   //if user joined some game server
                this->current_game = QString::fromUtf8(gameRecord.fullName) + " (" + QString::fromUtf8(gameserver_info) + ")";
            }
        }
        ui->currentlyPlayingTBox->setText(this->current_game);
    }
    else {      //if user changed his/her CUSTOM status
        this->custom_status = custom_status;
    }

    this->enter_in_critical_section(tID,!tID);  //if one thread is identifier with tID 0 and another with 1, then if we know one of their identifiers, it is easy to get another one with negation unary operator (!0 == 1, !1 == 0)

    QJsonObject user;
    user["connection"] = "0026";
    user["custom_status"] = this->custom_status;
    user["current_game"] = this->current_game;

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));
    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(300);
    qDebug() << "User status information is successfully sent!";

    packet.clear();
    m_Socket->waitForReadyRead(100);
    packet = m_Socket->readAll();
    object = QJsonDocument::fromJson(packet.constData());
    user = object.object();

    this->exit_from_critical_section(tID,!tID);
}

void MainWindow::on_currentStatusCBox_activated(const QString &arg1)
{
    if (QString::compare(arg1,this->custom_status)!=0) {
        this->custom_status=arg1;
        this->send_notification_message(0,this->custom_status.toStdString().c_str(),NULL,NULL);
    }
}

/*void MainWindow::setOverallStatus() {
    if (this->custom_status=="Online" || this->custom_status=="") {
        if (current_game=="") {
            ui->currentStatusCBox->setCurrentText(this->custom_status);
        }
        else {
            ui->currentStatusCBox->setCurrentText(this->current_game);
        }
    }
    else {
        if (current_game=="") {
            ui->currentStatusCBox->setCurrentText(this->custom_status);
        }
        else {
            ui->currentStatusCBox->setCurrentText(this->custom_status + " - " + this->current_game);
        }
    }
}*/

void MainWindow::refresh_games_list () {    //refreshes Table in "My Games" tab with added paths to games (if user adds path for some game, that means that he has that game and it is added on My Games list)
    ui->listWidget->clear();    //deletes all previous content of that list before adding new one (otherwise, there would be many duplicates)
    std::fstream file, file2;
    tPath recordPath;
    tGames recordGame;
    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    file2.open("gameslist.dat",std::ios::in | std::ios::binary);
    while (true) {
        file.read( (char*)&recordPath,sizeof(tPath) );
        if (file.eof()==true) {
            break;
        }
        if (strcmp(recordPath.path,"")==0) {
            continue;
        }
        short position = binarySearchWrapper(file2,recordPath.processName);
        file2.seekp(position*sizeof(tGames));
        file2.read( (char*)&recordGame,sizeof(tGames) );
        ui->listWidget->addItem(recordGame.fullName);
    }
    file.close();
    file.clear();
    file2.close();
}

void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    std::fstream file, file2;
    tPath recordPath;
    tGames recordGame;
    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    file2.open("gameslist.dat",std::ios::in | std::ios::binary);
    while (true) {
        file.read( (char*)&recordPath,sizeof(tPath) );
        if (file.eof()==true) {
            break;
        }
        if (strcmp(recordPath.path,"")==0) {
            continue;
        }
        short position = binarySearchWrapper(file2,recordPath.processName);
        file2.seekp(position*sizeof(tGames));
        file2.read( (char*)&recordGame,sizeof(tGames) );
        if (strcmp( item->text().toUtf8() , recordGame.fullName )==0) {
            start_program(recordPath.processName,NULL,NULL);
            break;
        }
    }
    file.close();
    file2.close();
}

short MainWindow::update_supported_games_list () {  //0...error, 1...current file is up to date, 2...new file has been received   //downloads new version of gameslist.dat if available
    int size;
    std::fstream file;
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    if (!file) {
        file.clear();
        size=0;
    }
    else {
        file.seekg(0,std::ios::end);
        size=file.tellg();
    }
    file.close();

    std::stringstream datetime;

    if (size!=0) {

#if defined (_WIN32)
        HANDLE hFile;
        TCHAR szBuf[MAX_PATH];
        const size_t cSize = strlen("gameslist.dat")+1;
        std::wstring wc( cSize, L'#' );
        mbstowcs( &wc[0], "gameslist.dat", cSize );
        hFile = CreateFile(wc.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        FILETIME ftCreate, ftAccess, ftWrite;
        SYSTEMTIME stUTC; //, stLocal;  //we won't use local times to avoid more complex comparison operations on server

        // Retrieve the file times for the file.
        GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite);

        // Convert the last-write time to local time.
        FileTimeToSystemTime(&ftWrite, &stUTC);
        //SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);  //we dont want to convert this into local time because then we would need to send proper tag of time zone

        // Build a string showing the date and time.
        StringCchPrintf(szBuf, MAX_PATH,
            TEXT("%04d-%02d-%02dT%02d:%02d:%02d"),
            stUTC.wYear, stUTC.wMonth, stUTC.wDay,
            stUTC.wHour, stUTC.wMinute, stUTC.wSecond);
        char tmp[25];
        wcstombs(tmp,szBuf,sizeof(tmp));
        datetime << tmp;
        CloseHandle(hFile);

#endif

#if defined (__linux__)
        struct tm* clock;               // create a time structure
        struct stat attrib;             // create a file attribute structure
        stat("gameslist.dat", &attrib);     // get the attributes of file gameslist.dat
        clock = gmtime(&(attrib.st_mtime));     //transform date of last modification time from Local Time Zone to GMT Time Zone
        //datetime << add_zeros_before(clock->tm_year,4) << "-" << add_zeros_before(clock->tm_mon+1) << "-" << add_zeros_before(clock->tm_mday) << "T" << add_zeros_before(clock->tm_hour) << ":" << add_zeros_before(clock->tm_min) << ":" << add_zeros_before(clock->tm_sec);   //concatenate date and time elements into string in valid format (US format) - this is replaced by functions that already exist (add_zeros_before(int,int) was user defined function)
        datetime << QString::number(clock->tm_year).rightJustified(4,'0') << "-" << QString::number(clock->tm_mon+1).rightJustified(2,'0') << "-" << QString::number(clock->tm_mday).rightJustified(2,'0') << "T" << QString::number(clock->tm_hour).rightJustified(2,'0') << ":" << QString::number(clock->tm_min).rightJustified(2,'0') << ":" << QString::number(clock->tm_sec).rightJustified(2,'0');   //concatenate date and time elements into string in valid format (US format)
#endif

    }
    else {
        datetime << "1990";
    }

    QJsonObject user;
    user["connection"] = "0018";
    user["size"] = size;
    user["datetime"] = datetime.str().c_str();  //if there is no file yet, then datetime of last modification won't be sent

    //qDebug() << "ADAWDWADAWDAW";
    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));
    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(1000);
    qDebug() << "Request for update info about gameslist.data has been successfully sent!";

    packet.clear();
    while(m_Socket->waitForReadyRead(100)) {
        packet += m_Socket->readAll();
    }
    object = QJsonDocument::fromJson(packet.constData());
    user = object.object();
    if (user["connection"] == "0019") { //file is up to date
        qDebug () << "You already have the latest version of list of supported games.";
        return 1;
    }
    else if (user["connection"] == "0020") {     //if received packet contains most recent file with supported games
        file.open("gameslist.dat",std::ios::out | std::ios::binary);
        QByteArray data;    //received stringified data (\0 became \u0000, numbers are not in numeric type) has to be converted back to
        data.setRawData( user["file"].toString().toStdString().c_str() , packet.length()-2-45 );    //size of stringified file data is equal to size of whole packet -2 (what are last 2 characters in packet: "} ) -41 (there are 45 characters in packet before file data (including "file" key))
        file.write( data.data(),user["size"].toString().toInt() );      //user["size"] is in non-numeric format, so it has first to be converted in string and then that string can be converted to integer (otherwise would 4/8 bytes of user["size"] be converted to integer)
        file.close();
        qDebug () << "File has been successfully downloaded!";
        return 2;
    }
    else {
        qDebug () << "Error has been occurred while receiving packet with most recent list of supported games.";
        return 0;   //error has occurred while receiving packet with most recent list of supported games
    }
}

void MainWindow::on_UserStatsButton_clicked()
{
    int row = ui->tableWidget->currentRow();
    QMessageBox msgBox;
    if (row == -1) {   //if any row isn't selected
        msgBox.setText("First select row with user for which you want get game statistics!");
        msgBox.exec();
        return;
    }
    this->showGameStats( ui->tableWidget->item(row, 0)->text() );
}

void MainWindow::showGameStats(QString email) {
    QJsonObject user;
    user["connection"] = "0027";
    user["email"] = email;

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));
    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(300);
    qDebug() << "Request for game statistics about " << user["email"] << " has been successfully sent!";

    packet.clear();

    while(m_Socket->waitForReadyRead(300)) {
        packet += m_Socket->readAll();
    }

    QJsonObject object2;
    QJsonDocument document;

    document = QJsonDocument::fromJson(packet.constData());
    object2 = document.object();

    QJsonValue value = object2.value("stats");
    QJsonArray array = value.toArray();

    QString statslist = "Username: " + email + "\r\n\r\n";
    if (array.count() == 0) {
        statslist += QString("This user hasn't played any game yet!");
    }
    else {
        statslist += QString("Game name").leftJustified(34, ' ') + QString("Time played").rightJustified(16, ' ') + "\r\n" + QString("__________________________________________________\r\n\r\n");
        for (int i=0 ; i < array.count() ; i++) {
            statslist += divide_on_multiple_lines(array[i].toObject().value("game").toString(),34) + seconds_to_HMS( array[i].toObject().value("time_played").toDouble() ).rightJustified(16, '.') + "\r\n";
        }
    }
    QMessageBox msgBox;
    msgBox.setWindowTitle( "Games statistics for user " + email );
    msgBox.setText(statslist);
    QFont font = QFont("Courier");
    msgBox.setFont(font);
    msgBox.exec();
}

void MainWindow::on_JoinFriendButton_clicked()
{
    int row = ui->tableWidget->currentRow();
    QMessageBox msgBox;
    if (row == -1) {   //if any row isn't selected
        msgBox.setText("First select row with user over whom you want to join gameserver!");
        msgBox.exec();
        return;
    }
    std::string game_info = ui->tableWidget->item(row,2)->text().toStdString();
    if (game_info.empty()==true) {
        msgBox.setText("User on which you have clicked isn't playing any game.");
        msgBox.exec();
        return;
    }
    int beginning_of_gameserver_info = game_info.find('(');
    if (beginning_of_gameserver_info==-1) {
        msgBox.setText("User on which you have clicked isn't playing on any gameserver");
        msgBox.exec();
        return;
    }

    std::fstream file;
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    tGames gameRecord;
    while (true) {
        file.read( (char*)&gameRecord,sizeof(tGames) );
        if (file.eof()==true) {
            file.close();
            file.clear();
            msgBox.setText("User is playing some game that isn't listed in your game library! Check if there's any newer version available.");
            msgBox.exec();
            return;
        }
        if (  strcmp( gameRecord.fullName , game_info.substr(0,beginning_of_gameserver_info-1).c_str() )==0  ) {
            break;
        }
    }
    int port_delimiter = game_info.find(':');
    start_program ( gameRecord.processName , game_info.substr(beginning_of_gameserver_info+1,port_delimiter).c_str() , game_info.substr(port_delimiter+1).c_str() );
    file.close();
}

void MainWindow::on_actionMy_Stats_triggered()
{
    this->showGameStats(this->m_Name);
}

void MainWindow::on_InstantChatButton_clicked()
{
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    short active_row = ui->tableWidget->currentRow();
    if(ui->tableWidget->item(active_row,0)->text() == m_Name) {
        QMessageBox msgbox;
        msgbox.setText("Get a friend mate!");
        msgbox.exec();
//        ui->ChatStatusLabel->setText("Get a friend mate!");
//        ui->ChatStatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    object["connection"] = "0021";
    object["username"] = ui->tableWidget->item(active_row,0)->text();
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(1000);
    if(m_Socket->waitForReadyRead(3000)) {
        packet = m_Socket->readAll();
        document = QJsonDocument::fromJson(packet.constData());
        object = document.object();

        if(object["connection"] == "0015") {
            QMessageBox msgbox;
            msgbox.setText("You can't chat with offline user.");
            msgbox.exec();
        }
    }
}

void MainWindow::on_actionDisconnect_triggered()
{
    this->close();
}
