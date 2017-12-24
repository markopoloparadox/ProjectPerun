#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addfriend.h"
#include <fstream>
#include <cstring>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <sstream>
#include <QMessageBox>

#if defined (__linux__)
#include <sys/stat.h>
#endif

#if defined (_WIN32)
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <QSettings>


HBITMAP GetScreenBmp(HDC hdc) {
    // Get screen dimensions
    int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create compatible DC, create a compatible bitmap and copy the screen using BitBlt()
    HDC hCaptureDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, nScreenWidth, nScreenHeight);
    HGDIOBJ hOld = SelectObject(hCaptureDC, hBitmap);
    BOOL bOK = BitBlt(hCaptureDC, 0, 0, nScreenWidth, nScreenHeight, hdc, 0, 0, SRCCOPY | CAPTUREBLT);

    SelectObject(hCaptureDC, hOld); // always select the previously selected object once done
    DeleteDC(hCaptureDC);
    return hBitmap;
}

void HandleHotkeys(void *arg) {
    MainWindow *mw = (MainWindow*)arg;
    Ui::MainWindow *ui = mw->ui;
    if (RegisterHotKey(NULL, 1, MOD_ALT | 0x4000, 0x42)) {
        qDebug() << "Hotkey is successfully registered, using MOD_NOREPEAT flag";
    }
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0) != 0) {
        QString processNameOfCurrentGame = ui->currentlyPlayingTBox->whatsThis();
        int extensionPosition = processNameOfCurrentGame.lastIndexOf(".exe");
        if (extensionPosition != -1) {
            processNameOfCurrentGame = processNameOfCurrentGame.mid(0, extensionPosition);
        }
        if (msg.message == WM_HOTKEY && !processNameOfCurrentGame.isEmpty()) {
            qDebug() << "WM_HOTKEY received";
            HDC hdc = GetDC(0);

            HBITMAP hBitmap = GetScreenBmp(hdc);

            mw->snd->setObjectName("screenshot");
            BITMAPINFO MyBMInfo = { 0 };
            MyBMInfo.bmiHeader.biSize = sizeof(MyBMInfo.bmiHeader);

            // Get the BITMAPINFO structure from the bitmap
            if (0 == GetDIBits(hdc, hBitmap, 0, 0, NULL, &MyBMInfo, DIB_RGB_COLORS)) {
                qDebug() << "error" << endl;
            }

            // create the bitmap buffer
            BYTE* lpPixels = new BYTE[MyBMInfo.bmiHeader.biSizeImage];

            // Better do this here - the original bitmap might have BI_BITFILEDS, which makes it
            // necessary to read the color table - you might not want this.
            MyBMInfo.bmiHeader.biCompression = BI_RGB;

            // get the actual bitmap buffer
            if (0 == GetDIBits(hdc, hBitmap, 0, MyBMInfo.bmiHeader.biHeight, (LPVOID)lpPixels, &MyBMInfo, DIB_RGB_COLORS)) {
                qDebug() << "error2" << endl;
            }

            if (lpPixels) {
                QString currentDate = QDateTime::currentDateTime().toString("dd-MM-yyyy_hh-mm-ss");
                QString imageName = processNameOfCurrentGame + "-" + currentDate + ".bmp";
                qDebug() << "Screenshot will be stored with following name: " << imageName;
                std::fstream file;
                file.open(imageName.toStdString().c_str(), std::ios::out | std::ios::binary);
                BITMAPFILEHEADER hdr;
                hdr.bfOffBits = 54;
                hdr.bfReserved1 = 0;
                hdr.bfReserved2 = 0;
                hdr.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) + GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN) * 3;
                hdr.bfType = 0x4d42;

                file.write((char*)&hdr, sizeof(BITMAPFILEHEADER));
                file.write((char*)&MyBMInfo, sizeof(MyBMInfo));
                file.write((char*)lpPixels, MyBMInfo.bmiHeader.biSizeImage);

                file.close();
            }

            DeleteObject(hBitmap);
            ReleaseDC(NULL, hdc);
            delete[] lpPixels;
        }
    }
}

QString MainWindow::getFileLocationFromRegistryKey(QString registryKey) {
    int keynamePosition = registryKey.lastIndexOf('\\') + 1;
    QString path = registryKey.mid(0, keynamePosition);
    QString keyname = registryKey.mid(keynamePosition);

    QSettings settings(
        path,
        QSettings::NativeFormat
    );
    if (keyname.length() == 0) {
        return settings.value("Default").toString();
    }
    else {
        return settings.value(keyname).toString();
    }
}

void MainWindow::autoDetectGames() {
    tGames gameRecord;
    tPath gamePath;
    std::fstream fileAllGames;
    std::fstream fileMyGames;
    QMap<QString, tPath> assocList;
    fileHandlingMutex.lock();
    fileMyGames.open("gamepath.dat", std::ios::in | std::ios::binary);
    while (true) {
        fileMyGames.read((char*)&gamePath, sizeof(tPath));
        if (fileMyGames.eof()==true) {
            break;
        }
        assocList.insert(gamePath.processName, gamePath);
    }
    fileMyGames.clear();
    fileMyGames.close();
    fileAllGames.open("gameslist.dat", std::ios::in | std::ios::out | std::ios::binary);    //the content will be removed when file will be released
    while (true) {
        bool found = false;
        fileAllGames.read((char*)&gameRecord, sizeof(tGames));
        if (fileAllGames.eof()==true) {
            break;
        }
        QString lastKnownLocation = "";
        if (assocList.contains(gameRecord.processName)) {
            lastKnownLocation = assocList.find(gameRecord.processName)->path;
            if (QFile::exists(lastKnownLocation + gameRecord.processName)) {
                found = true;
            }
        }
        if (!found) {
            QString gameLocationFromRegistry = this->getFileLocationFromRegistryKey(gameRecord.registryKeyFullname);
            if (gameLocationFromRegistry.isEmpty()) {
                if (!lastKnownLocation.isEmpty()) {
                    assocList.remove(gameRecord.processName);
                }
            }
            else {
                if (!gameLocationFromRegistry.endsWith(".exe")) {
                    if (!gameLocationFromRegistry.endsWith("\\")) {
                        gameLocationFromRegistry.append("\\");
                    }
                }
                else {
                    gameLocationFromRegistry.resize(gameLocationFromRegistry.lastIndexOf('\\') + 1);
                }
                if (QFile::exists(gameLocationFromRegistry + gameRecord.processName)) {
                    strcpy(gamePath.processName, gameRecord.processName);
                    strcpy(gamePath.path, gameLocationFromRegistry.toStdString().c_str());
                    strcpy(gamePath.customExecutableParameters, "");
                    assocList.insert(gameRecord.processName, gamePath);
                }
            }
        }
    }
    fileMyGames.open("gamepath.dat", std::ios::out | std::ios::binary);
    for (QMap<QString, tPath>::iterator i = assocList.begin(); i != assocList.end(); i++) {
        fileMyGames.write((char*)&i.value(), sizeof(tPath));
    }
    fileMyGames.close();
    fileHandlingMutex.unlock();
}
#endif

MainWindow::MainWindow(QTcpSocket *socket, bool aMode, QString name, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_Name(name)
{
    ui->setupUi(this);

    this->setWindowTitle("Perun (Logged in as " + name + ")");
    this->m_Socket = socket;
    this->adminMode = aMode;

    this->snd = new QSound("");
    connect(this->snd, &QSound::objectNameChanged, QCoreApplication::instance(),
                [&]()
                {
                    if (!this->snd->objectName().isEmpty()) {
                        if (this->snd->objectName() == "screenshot") {
                            this->snd->play(":/sound/camera_shutter_sound.wav");
                        }
                        else {
                            this->snd->play(":/sound/message_notification_sound.wav");
                        }
                        this->snd->setObjectName("");
                    }
                },
                Qt::QueuedConnection
    );

    connect(m_Socket, SIGNAL(readyRead()), this, SLOT(onTcpMessageReceived()));
    this->request_friends_list();

    std::fstream file;
    fileHandlingMutex.lock();
    file.open("gamepath.dat",std::ios::app); //create file if it does not exists; otherwise do nothing (it won't be changed)
    file.close();
    fileHandlingMutex.unlock();

    this->check_if_newer_games_list_exist();          //checks if there's any newer version of gameslist.dat (if there is, server sends it to us)

    ui->currentStatusCBox->addItem("Online");
    ui->currentStatusCBox->addItem("Away From Keyboard");
    ui->currentStatusCBox->addItem("Busy");
    ui->currentStatusCBox->setCurrentText( this->custom_status = QString::fromUtf8("Online") );
    ui->currentlyPlayingTBox->setText( this->current_game = "" );
    ui->currentlyPlayingTBox->setWhatsThis("");
    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
    ui->currentlyPlayingTBox->setFocus();

    gameActivityListenerThread = new std::thread (ListenGameActivity, this);

    globalShortcutListenerThread = new std::thread(HandleHotkeys, this);
}

MainWindow::~MainWindow(){      //destructor isn't called when form is closed because this form isn't called with Qt::WA_DeleteOnClose attribute
    m_Socket->disconnectFromHost();
    m_Socket->close();
    delete m_Socket;
    delete snd;
    delete ui;
    delete gameActivityListenerThread;     //if destructor is called, this would cause error because active thread must previously be finished and then catched with join() method
    delete globalShortcutListenerThread;
}

void MainWindow::on_AddFriendButton_clicked(){
    AddFriend* addFriendWin = new AddFriend(m_Socket);
    addfriendbox = addFriendWin->ui;
    addFriendWin->show();
}

void MainWindow::request_friends_list(){
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0011";
    document.setObject(object);
    packet = document.toJson(QJsonDocument::Compact);

    m_Socket->write(packet);
    m_Socket->flush();
}

void MainWindow::get_friends_list(QJsonObject message){
    QJsonArray friends = message.value("friends").toArray();

    while (ui->tableWidget->rowCount() < friends.count())
        ui->tableWidget->insertRow(ui->tableWidget->rowCount());

    for (int i = 0; i < friends.count(); i++) {
        QJsonObject friendInfo = friends[i].toObject();
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(friendInfo.value("email").toString()));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(friendInfo.value("custom_status").toString()));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(friendInfo.value("current_game").toString()));
    }

    QJsonArray friendRequests = message.value("friend_requests").toArray();
    for (int i = 0; i < friendRequests.count(); i++) {
        this->process_friend_request(friendRequests[i].toString());
    }
}

void MainWindow::process_friend_request(QString username) {
    QMessageBox msgbox(QMessageBox::Icon::NoIcon, "New friend request", "User " + username + " wants to be your friend. Do you want to accept it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Ignore);
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    switch (msgbox.exec()) {
        case QMessageBox::StandardButton::Yes:
            object["accept"] = true;
            break;
        case QMessageBox::StandardButton::No:
            object["accept"] = false;
            break;
    }
    if (object.contains("accept")) {
        object["connection"] = "0015";
        object["email"] = username;
        document.setObject(object);
        packet = document.toJson(QJsonDocument::Compact);

        m_Socket->write(packet);
        m_Socket->flush();
    }
}

void MainWindow::on_actionConfigure_game_library_triggered()
{
    gameLibWin = new gamelibrary(this);
    gameLibWin->show();
}

void ListenGameActivity (void *arg) {    //my compiler does not not allow that threaded function is class member (i.e. method), so this is workaround - people say that that it compiler's bug
    MainWindow *mainClass = static_cast<MainWindow*>(arg);
    mainClass->check_game_status();
}

void MainWindow::check_game_status()
{
    char *running_game, *gameserver_info;
    while (true) {
        gameserver_info = NULL; //= new char [22];

        fileHandlingMutex.lock();

        running_game = game_running_in_background();

        fileHandlingMutex.unlock();

        std::fstream file;
        if (running_game != NULL) {
            if (this->adminMode == true) {
                //gameserver_info = NULL;
                file.open("lock.dat",std::ios::app);    //file is now locked - that means that network tracing will now start because game was detected
                //gameserver_info = found_gameserver_address(running_game); //this line is commented because some time has to go after lock file is locked (because background service is checking it every 10 seconds, so in many cases it won't be detected instantly and program would try to find IP address of gameserver in file that does not even exist (or in file with data from previous time when network traffic was tracked))
            }
            send_notification_message(1,NULL,running_game,gameserver_info);
        }
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

void MainWindow::send_notification_message (short tID, const char* custom_status=NULL, char* played_game_name=NULL, char* gameserver_info=NULL) {  //notifies friends that you have started playing some game (or you stopped playing) XOR you have changed your custom status (note on exlusive or because function is separately called when user changed custom status and when his/her game status was changed
/*    if (custom_status==NULL && played_game_name==NULL && gameserver_info==NULL) {   //if we haven't changed custom status and are not playing on any game
        if (this->current_game == "") {     //if we haven't also played anything 10 seconds before - we don't need to notify others that we are not playing anything because they already know that
            return;
        }
    }   */

    if (custom_status==NULL) {              //if user's GAME status was changed
        if (played_game_name==NULL) {           //if user stoped playing current game
            this->current_game = "";
        }
        else {      //if user started playing game or remained playing current game on another game server
            std::fstream file;
            tGames gameRecord;

            fileHandlingMutex.lock();

            file.open("gameslist.dat",std::ios::in | std::ios::binary);
            int position = binarySearchWrapper(file,played_game_name);
            file.seekg(position*sizeof(tGames));
            file.read( (char*)&gameRecord,sizeof(tGames) );
            file.close();

            fileHandlingMutex.unlock();

            if (gameserver_info==NULL) {        //if user started game but isn't on any server or (s)he disconnected from current and isn't playing on any right now
                this->current_game = QString::fromUtf8(gameRecord.fullName);
            }
            else if (gameserver_info!=NULL) {   //if user joined some game server
                this->current_game = QString::fromUtf8(gameRecord.fullName) + " (" + QString::fromUtf8(gameserver_info) + ")";
            }
        }
        ui->currentlyPlayingTBox->setText(this->current_game);
        ui->currentlyPlayingTBox->setWhatsThis(played_game_name);
    }
    else {      //if user changed his/her CUSTOM status
        this->custom_status = custom_status;
    }

    QJsonObject user;
    user["connection"] = "0026";
    user["custom_status"] = this->custom_status;
    user["current_game"] = this->current_game;

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->flush();
    qDebug() << "User status information is successfully sent!";
}

void MainWindow::on_currentStatusCBox_activated(const QString &arg1)
{
    if (QString::compare(arg1,this->custom_status)!=0) {
        this->custom_status=arg1;
        this->send_notification_message(0,this->custom_status.toStdString().c_str(),NULL,NULL);
    }
}

void MainWindow::refresh_games_list () {    //refreshes Table in "My Games" tab with added paths to games (if user adds path for some game, that means that he has that game and it is added on My Games list)
    ui->listWidget->clear();    //deletes all previous content of that list before adding new one (otherwise, there would be many duplicates)
    std::fstream file, file2;
    tPath recordPath;
    tGames recordGame;

    fileHandlingMutex.lock();

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
        if (position == -1) {
            continue;
        }
        file2.seekp(position*sizeof(tGames));
        file2.read( (char*)&recordGame,sizeof(tGames) );
        ui->listWidget->addItem(recordGame.fullName);
    }
    file.close();
    file.clear();
    file2.close();

    fileHandlingMutex.unlock();

}

void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    std::fstream file, file2;
    tPath recordPath;
    tGames recordGame;

    fileHandlingMutex.lock();

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
        if (position==-1) { // some game can appear in gamepath.dat as registered on user's computer, but it's not in the list of supported games if support for that game is eventaully removed or process name is changed
            continue;
        }
        file2.seekp(position*sizeof(tGames));
        file2.read( (char*)&recordGame,sizeof(tGames) );
        if (strcmp( item->text().toUtf8() , recordGame.fullName )==0) {
            file.close();
            file2.close();
            fileHandlingMutex.unlock();
            this->start_program(recordPath.processName, NULL, NULL);
            return;
        }
    }
    file.close();
    file2.close();

    fileHandlingMutex.unlock();

}

void MainWindow::update_supported_games_list (QJsonObject message, int packetSize) {
    std::fstream file;
    fileHandlingMutex.lock();
    file.open("gameslist.dat",std::ios::out | std::ios::binary);
    QByteArray data;    //received stringified data (\0 became \u0000 and numbers are not in numeric type) has to be converted back to
    data.setRawData( message["file"].toString().toStdString().c_str() , packetSize-2-45 );    //size of stringified file data is equal to size of whole packet -2 (what are last 2 characters in packet: "} ) -41 (there are 45 characters in packet before file data (including "file" key))
//char* ptr = new char [message["size"].toString().toInt()];
//        memcpy(ptr,data,message["size"].toString().toInt());     //data.data()   is causing program to crash
    file.write( message["file"].toString().toStdString().c_str() , message["size"].toString().toInt() );
    file.close();
    fileHandlingMutex.unlock();
    qDebug () << "File has been successfully downloaded!";
}

void MainWindow::on_UserStatsButton_clicked()
{
    QMessageBox msgBox;
    int row = ui->tableWidget->currentRow();
    if (row == -1) {   //if any row isn't selected
        msgBox.setText("First select row with user for which you want get game statistics!");
        msgBox.exec();
        return;
    }
    this->requestGameActivityInfo(ui->tableWidget->item(row, 0)->text());
}

void MainWindow::requestGameActivityInfo(QString email)
{
    QJsonObject user;
    user["connection"] = "0027";
    user["email"] = email;

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->flush();
    qDebug() << "Request for game statistics about " << user["email"].toString() << " has been successfully sent!";
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
    QString game_info = ui->tableWidget->item(row,2)->text();
    if (game_info.isEmpty()) {
        msgBox.setText("User on which you have clicked isn't playing any game.");
        msgBox.exec();
        return;
    }
    std::string gameName = extractGameNameOnly(game_info).toStdString();
    if (gameName == game_info.toStdString()) {
        msgBox.setText("User on which you have clicked isn't playing on any gameserver");
        msgBox.exec();
        return;
    }
    std::string serverIpAndPortInsideBrackets = game_info.replace(0, gameName.length()+2, "").toStdString();

    std::fstream file;

    fileHandlingMutex.lock();

    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    tGames gameRecord;
    while (true) {
        file.read( (char*)&gameRecord,sizeof(tGames) );
        if (file.eof()==true) {
            file.close();
            file.clear();
            fileHandlingMutex.unlock();
            msgBox.setText("User is playing some game that isn't listed in your game library! Check if there's any newer version of supported games list available.");
            msgBox.exec();
            return;
        }
        if (  strcmp( gameRecord.fullName , gameName.c_str() )==0  ) {
            break;
        }
    }
    file.close();
    fileHandlingMutex.unlock();

    int delimiterPosition = serverIpAndPortInsideBrackets.find_first_of(':');
    start_program( gameRecord.processName , serverIpAndPortInsideBrackets.substr(0, delimiterPosition).c_str() , serverIpAndPortInsideBrackets.substr(delimiterPosition+1,serverIpAndPortInsideBrackets.length()-delimiterPosition-2).c_str() );
}

void MainWindow::on_actionMy_Stats_triggered()
{
    this->requestGameActivityInfo(this->m_Name);
}

void MainWindow::on_InstantChatButton_clicked()
{
    short active_row = ui->tableWidget->currentRow();
    if (active_row == -1) {
        QMessageBox msgBox;
        msgBox.setText("First select row with user with which you want to chat!");
        msgBox.exec();
        return;
    }
    QString username = ui->tableWidget->item(active_row,0)->text();
    if (username == m_Name) {
        QMessageBox msgbox;
        msgbox.setText("Get a friend mate!");
        msgbox.exec();
        return;
    }
    if (ui->tableWidget->item(active_row,1)->text() == "Offline") {
        QMessageBox msgbox;
        msgbox.setText("You can't chat with offline user.");
        msgbox.exec();
        return;
    }
    ChatBox* chatbox = new ChatBox(m_Socket, username, this);
    privateChatMap.insert(username, chatbox);
    chatbox->show();
}

void MainWindow::on_actionDisconnect_triggered()
{
    this->close();
    this->m_Socket->close();
    QApplication::exit(1);      //restart application (show login screen again)
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
    this->m_Socket->close();
    QApplication::exit(0);      //shut down application
}

void MainWindow::on_tableWidget_cellDoubleClicked(int row, int column)
{
    this->on_InstantChatButton_clicked();
}

void MainWindow::onTcpMessageReceived() {
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    packet = m_Socket->readAll();

    std::vector<int> ranges;
    int packetSize = packet.size();
    byte numOfConsecutiveEscapeChars = 0;
    int curlyBracketsState = 0;
    int rangeSize = 0;
    for (int i=0; i<packetSize; i++) {
        char currChar = packet.at(i);
        if (currChar == '\\') {
            numOfConsecutiveEscapeChars++;
        }
        else {
            if (numOfConsecutiveEscapeChars % 2 == 0) {
                if (currChar == '{') {
                    curlyBracketsState++;
                }
                else if (currChar == '}') {
                    if (--curlyBracketsState == 0) {
                        ranges.push_back(i+1);
                        rangeSize++;
                    }
                }
            }
            numOfConsecutiveEscapeChars = 0;
        }
    }
    int lowerLimit = 0;
    for (int i=0; i<rangeSize; i++) {
        int upperLimit = ranges.at(i);

        document = QJsonDocument::fromJson(packet.mid(lowerLimit, upperLimit - lowerLimit).constData());
        object = document.object();

        QString messageCode = object["connection"].toString();
        switch (messageCode.toInt()) {
            case 5:
                addfriendbox->StatusLabel->setText("Sorry, that user does not seem to exist in our database!");
                addfriendbox->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
                break;
            case 9:
                addfriendbox->StatusLabel->setText("Friend request sent!");
                addfriendbox->StatusLabel->setStyleSheet("QLabel { background-color : white; color : green; }");
                break;
            case 10:
                addfriendbox->StatusLabel->setText("That user is already your friend!");
                addfriendbox->StatusLabel->setStyleSheet("QLabel { background-color : white; color : yellow; }");
                break;
            case 12:
                this->get_friends_list(object);
                break;
            case 13:
                addfriendbox->StatusLabel->setText("Don't be silly!");
                addfriendbox->StatusLabel->setStyleSheet("QLabel { background-color : white; color : yellow; }");
                break;
            case 14:
                addfriendbox->StatusLabel->setText("Your previous request is still pending!");
                addfriendbox->StatusLabel->setStyleSheet("QLabel { background-color : white; color : yellow; }");
                break;
            case 16:
                this->refresh_friends_list(object);
                break;
            case 17:
                this->process_friend_request(object["email"].toString());
                break;
            case 19:
                qDebug () << "You already have the latest version of list of supported games.";
                this->autoDetectGames();

                this->refresh_games_list();     //refreshes table in "My Games" tab with games that user owns (for which (s)he defined path)

                if (!initialGamesListCheckingDone) {
                    initialGamesListCheckingDone = true;
                }
                else {
                    QMessageBox msgBox;
                    msgBox.setText("Current list of supported games is up to date!");
                    msgBox.exec();
                }
                break;
            case 20:
                this->update_supported_games_list(object, packet.size());
                this->autoDetectGames();

                this->refresh_games_list();     //refreshes table in "My Games" tab with games that user owns (for which (s)he defined path)

                if (!initialGamesListCheckingDone) {
                    initialGamesListCheckingDone = true;
                }
                else {
                    this->autoDetectGames();
                    this->refresh_games_list();
                    gameLibWin->fill_table();
                    QMessageBox msgBox;
                    msgBox.setText("Newer version of list of supported games has been found and received!");
                    msgBox.exec();
                }
                break;
            case 22:
            case 23:
                this->process_new_chat_message(object);
                break;
            case 28:
                this->showGameStats(object);
                break;
            default:
                qDebug() << "Unprocessed message: " << packet;
        }

        lowerLimit = upperLimit;
    }
}

void MainWindow::process_new_chat_message(QJsonObject message) {
    if (message["isprivate"].toBool()) {
        QString sender = message["chatid"].toString();
        ChatBox* chatbox;
        if (chatbox = privateChatMap[sender]) {
            chatbox->Update(message);
            chatbox->show();
            return;
        }
        chatbox = new ChatBox(message, m_Socket, sender, this);
        privateChatMap.insert(sender, chatbox);
        chatbox->show();
    }
    else {
        QString chatRoomId = message["chatid"].toString();
        ChatBox* chatbox;;
        if (chatbox = groupChatMap[chatRoomId]) {
            chatbox->Update(message);
            chatbox->show();
            return;
        }
        chatbox = new ChatBox(message, m_Socket, chatRoomId, this, true);
        groupChatMap.insert(chatRoomId, chatbox);
        chatbox->show();
    }
    snd->setObjectName("message");
}

void MainWindow::refresh_friends_list(QJsonObject message) {
    int numRows = ui->tableWidget->rowCount();
    QString username = message["email"].toString();
    QString newCustomStatus = message["custom_status"].toString();
    QString newGameStatus = message["current_game"].toString();
    if (addfriendbox != NULL && addfriendbox->EmailLineEdit->text() == username) {
        addfriendbox->StatusLabel->setText("Specified user is now your friend!");
        addfriendbox->StatusLabel->setStyleSheet("QLabel { background-color : white; color : green; }");
    }
    for(int i=0; i < numRows; i++) {
        if (ui->tableWidget->item(i, 0)->text() == username) {
            ChatBox* chatbox = privateChatMap[username];
            if (chatbox != NULL) {
                QString oldCustomStatus = ui->tableWidget->item(i, 1)->text();
                QString oldGameName = extractGameNameOnly(ui->tableWidget->item(i, 2)->text());
                QString newGameName = extractGameNameOnly(newGameStatus);
                if (oldCustomStatus != newCustomStatus) {
                    if (oldCustomStatus == "Offline") {
                        chatbox->Update(username + " is now online!");
                    }
                    else if (newCustomStatus == "Offline") {
                        chatbox->Update(username + " is now offline..");
                    }
                    else {
                        chatbox->Update(((QString)"%1 has changed their status from \"%2\" to \"%3\"").arg(username, oldCustomStatus, newCustomStatus));
                    }
                }
                else if (oldGameName != newGameName) {
                    if (oldGameName.isEmpty()) {
                        chatbox->Update(((QString)"%1 is now playing \"%2\"").arg(username, newGameName));
                    }
                    else {
                        chatbox->Update(((QString)"%1 has stopped playing \"%2\"").arg(username, oldGameName));
                    }
                }
            }
            ui->tableWidget->item(i, 1)->setText(newCustomStatus);
            ui->tableWidget->item(i, 2)->setText(newGameStatus);
            return;
        }
    }
    ui->tableWidget->insertRow(numRows);
    ui->tableWidget->setItem(numRows, 0, new QTableWidgetItem(username));
    ui->tableWidget->setItem(numRows, 1, new QTableWidgetItem(newCustomStatus));
    ui->tableWidget->setItem(numRows, 2, new QTableWidgetItem(newGameStatus));
}

void MainWindow::showGameStats(QJsonObject object) {
    QJsonValue value = object.value("stats");
    QJsonArray array = value.toArray();

    QString email = object.value("email").toString();
    QString statslist = "Username: " + email + "\r\n\r\n";
    if (array.count() == 0) {
        statslist += QString("This user hasn't played any game yet!");
    }
    else {
        statslist += "<thead><tr><th>Game name</th><th align=\"right\">Time played</th></tr></thead><tbody>";
        QJsonArray sortedArray;
        int numOfPlayedGames = array.count();
        for (int i=0 ; i < numOfPlayedGames ; i++) {
            double maxTime = 0;
            int maxIndex;
            for (int j=0 ; j < numOfPlayedGames-i ; j++) {
                double currTime = array[j].toObject().value("time_played").toDouble();
                if (maxTime < currTime) {
                    maxTime = currTime;
                    maxIndex = j;
                }
            }
            sortedArray.append(array[maxIndex]);
            array.removeAt(maxIndex);
        }
        for (int i=0 ; i < numOfPlayedGames ; i++) {
            statslist += "<tr><td>" + sortedArray[i].toObject().value("game").toString() + "</td><td align=\"right\" valign=\"middle\">" + seconds_to_HMS(sortedArray[i].toObject().value("time_played").toDouble()) + "</td></tr>";
        }
    }
    QMessageBox msgBox;
    msgBox.setWindowTitle( "Games statistics for user " + email );
    msgBox.setText("<table border=\"2\" cellspacing=\"0\">" + statslist + "</tbody></table>");
    msgBox.exec();
}

void MainWindow::check_if_newer_games_list_exist() {
    int size;
    std::fstream file;

    fileHandlingMutex.lock();
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
        datetime << QString::number(clock->tm_year).rightJustified(4,'0') << "-" << QString::number(clock->tm_mon+1).rightJustified(2,'0') << "-" << QString::number(clock->tm_mday).rightJustified(2,'0') << "T" << QString::number(clock->tm_hour).rightJustified(2,'0') << ":" << QString::number(clock->tm_min).rightJustified(2,'0') << ":" << QString::number(clock->tm_sec).rightJustified(2,'0');   //concatenate date and time elements into string in valid format (US format)
#endif

    }
    else {
        datetime << "1990";
    }
    fileHandlingMutex.unlock();

    QJsonObject user;
    user["connection"] = "0018";
    user["size"] = size;
    user["datetime"] = datetime.str().c_str();  //if there is no file yet, then datetime of last modification is undefined and very old date is sent

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->flush();
    qDebug() << "Request for update info about gameslist.dat has been successfully sent!";
}

void MainWindow::start_program (const char* prog_name, const char* ip, const char* port) {    //start game which server flagged as supported (in gameslist.dat file) and which path is defined (in gamepath.dat file)
    std::stringstream command;
    std::fstream file;
    tGames gameRecord;

    fileHandlingMutex.lock();
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    file.seekg( binarySearchWrapper(file,prog_name)*sizeof(tGames) , std::ios::beg );
    file.read( (char*)&gameRecord,sizeof(tGames) );
    file.close();

    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    tPath pathRecord;
    bool pathNotDefined=true;
    while (true) {
        file.read( (char*)&pathRecord,sizeof(tPath) );
        if (file.eof()==true) {
            pathNotDefined=false;
            break;
        }
        if (strcmp(pathRecord.processName,prog_name)==0) {
            break;
        }
    }

    file.close();
    fileHandlingMutex.unlock();
    if (pathNotDefined==false || strcmp(pathRecord.path,"")==0) {   //record about some game can be stored locally if user removes game from his list of games (if he set game path to nullString)
        QMessageBox msgBox;
        msgBox.setText("Define game's path in settings in order to join game over your friend");
        msgBox.exec();
        file.clear();
        return;
    }
    QString launchArguments = gameRecord.multiplayerCommandLineArguments;
#if defined (_WIN32)
    command << "cd /d \"" << pathRecord.path << "\" && " << "start ";
#endif
#if defined (__linux__)
    std::string path = pathRecord.path;
    path.replace(path.find("/home"),5,"~");             //replaces "/home" part of directory path with "~" because that's the only way how programs can be run when using absolute path
    command << path.c_str() << "/" << prog_name;
#endif

    if (ip!=NULL && strcmp(gameRecord.multiplayerCommandLineArguments,"\0")!=0) {   //if in this function are passed ip address and port of some remote server and if there exists a way to join specific gameserver in game directly via command line
        launchArguments = launchArguments.replace("%%exe%%", prog_name);
        launchArguments = launchArguments.replace("%%ip%%", ip);
        launchArguments = launchArguments.replace("%%port%%", port);
        command << " " << launchArguments.toStdString().c_str();
    }
    else {
        command << prog_name;
    }
    command << " " << pathRecord.customExecutableParameters;

    system(command.str().c_str());  //executing
}
