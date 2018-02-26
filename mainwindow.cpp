#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "AddFriendWindow.h"
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


HBITMAP getScreenBmp(HDC hdc) {
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

void handleHotkeys(void *arg) {
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

            HBITMAP hBitmap = getScreenBmp(hdc);

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

    for (QString firstLevelPath : {"HKEY_LOCAL_MACHINE\\", "HKEY_CURRENT_USER\\"}) {
        for (QString secondLevelPath : {"Software\\Wow6432Node\\", "Software\\"}) {
            QSettings settings(
                firstLevelPath + secondLevelPath + path,
                QSettings::NativeFormat
            );
            QString result = settings.value(keyname.isEmpty() ? "Default" : keyname).toString();
            if (!result.isEmpty()) {
                return result;
            }
        }
    }
    return "";
}

void MainWindow::autoDetectGames() {
    tGames gameRecord;
    tPath gamePath;
    std::fstream fileAllGames;
    std::fstream fileMyGames;
    QMap<QString, tPath> assocList;
    this->fileHandlingMutex.lock();
    fileMyGames.open("gamepath.dat", std::ios::in | std::ios::binary);
    while (true) {
        fileMyGames.read((char*)&gamePath, sizeof(tPath));
        if (fileMyGames.eof()) {
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
        if (fileAllGames.eof()) {
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
            if (strcmp(gameRecord.registryKeyFullname, "") != 0) {
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
    }
    fileMyGames.open("gamepath.dat", std::ios::out | std::ios::binary);
    for (QMap<QString, tPath>::iterator i = assocList.begin(); i != assocList.end(); i++) {
        fileMyGames.write((char*)&i.value(), sizeof(tPath));
    }
    fileMyGames.close();
    this->fileHandlingMutex.unlock();
}
#endif

MainWindow::MainWindow(QTcpSocket *socket, bool adminMode, QString username, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    username(username),
    socket(socket),
    adminMode(adminMode),
    snd(new QSound(""))
{
    this->ui->setupUi(this);

    this->setWindowTitle("Perun (Logged in as " + username + ")");

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

    connect(socket, SIGNAL(readyRead()), this, SLOT(onTcpMessageReceived()));
    this->requestFriendsList();

    std::fstream file;
    this->fileHandlingMutex.lock();
    file.open("gamepath.dat",std::ios::app); //create file if it does not exists; otherwise do nothing (it won't be changed)
    file.close();
    this->fileHandlingMutex.unlock();

    this->checkIfNewerGamesListExist();          //checks if there's any newer version of gameslist.dat (if there is, server sends it to us)

    this->ui->currentStatusCBox->addItem("Online");
    this->ui->currentStatusCBox->addItem("Away From Keyboard");
    this->ui->currentStatusCBox->addItem("Busy");
    this->ui->currentStatusCBox->setCurrentText( this->customStatus = QString::fromUtf8("Online") );
    this->ui->currentlyPlayingTBox->setText( this->currentGame = "" );
    this->ui->currentlyPlayingTBox->setWhatsThis("");
    this->ui->tableWidget->verticalHeader()->hide();
    this->ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    this->ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    this->ui->currentlyPlayingTBox->setFocus();

    this->gameActivityListenerThread = new std::thread(listenGameActivity, this);

    this->globalShortcutListenerThread = new std::thread(handleHotkeys, this);
}

MainWindow::~MainWindow(){      //destructor isn't called when form is closed because this form isn't called with Qt::WA_DeleteOnClose attribute
    this->socket->disconnectFromHost();
    this->socket->close();
    delete this->socket;
    delete this->snd;
    delete this->ui;
    delete this->gameActivityListenerThread;     //if destructor is called, this would cause error because active thread must previously be finished and then catched with join() method
    delete this->globalShortcutListenerThread;
}

void MainWindow::on_addFriendButton_clicked(){
    AddFriendWindow* addFriendWin = new AddFriendWindow(this->socket);
    this->addFriendBox = addFriendWin->ui;
    addFriendWin->show();
}

void MainWindow::requestFriendsList(){
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0011";
    document.setObject(object);
    packet = document.toJson(QJsonDocument::Compact);

    this->socket->write(packet);
    this->socket->flush();
}

void MainWindow::getFriendsList(QJsonObject message){
    QJsonArray friends = message.value("friends").toArray();

    while (this->ui->tableWidget->rowCount() < friends.count())
        this->ui->tableWidget->insertRow(this->ui->tableWidget->rowCount());

    for (int i = 0; i < friends.count(); i++) {
        QJsonObject friendInfo = friends[i].toObject();
        this->ui->tableWidget->setItem(i, 0, new QTableWidgetItem(friendInfo.value("email").toString()));
        this->ui->tableWidget->setItem(i, 1, new QTableWidgetItem(friendInfo.value("custom_status").toString()));
        this->ui->tableWidget->setItem(i, 2, new QTableWidgetItem(friendInfo.value("current_game").toString()));
    }

    QJsonArray friendRequests = message.value("friend_requests").toArray();
    for (int i = 0; i < friendRequests.count(); i++) {
        this->processFriendRequest(friendRequests[i].toString());
    }
}

void MainWindow::processFriendRequest(QString username) {
    QMessageBox msgBox(QMessageBox::Icon::NoIcon, "New friend request", "User " + username + " wants to be your friend. Do you want to accept it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Ignore);
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    switch (msgBox.exec()) {
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

        this->socket->write(packet);
        this->socket->flush();
    }
}

void MainWindow::on_actionConfigureGameLibrary_triggered()
{
    this->gameLibWin = new GameLibraryWindow(this);
    this->gameLibWin->show();
}

void listenGameActivity (void *arg) {    //my compiler does not not allow that threaded function is class member (i.e. method), so this is workaround - people say that that it compiler's bug
    MainWindow *mainClass = static_cast<MainWindow*>(arg);
    mainClass->checkGameStatus();
}

void MainWindow::checkGameStatus()
{
    char *runningGame, *gameserverInfo;
    while (true) {
        gameserverInfo = NULL; //= new char [22];

        this->fileHandlingMutex.lock();

        runningGame = getNameOfGameRunningInBackground();

        this->fileHandlingMutex.unlock();

        std::fstream file;
        if (runningGame != NULL) {
            if (this->adminMode) {
                //gameserverInfo = NULL;
                file.open("lock.dat",std::ios::app);    //file is now locked - that means that network tracing will now start because game was detected
                //gameserverInfo = foundGameserverAddress(runningGame); //this line is commented because some time has to go after lock file is locked (because background service is checking it every 10 seconds, so in many cases it won't be detected instantly and program would try to find IP address of gameserver in file that does not even exist (or in file with data from previous time when network traffic was tracked))
            }
            sendNotificationMessage(1,NULL,runningGame,gameserverInfo);
        }
        sleep(10);
        while (runningGame!=NULL && strcmp(getNameOfGameRunningInBackground(runningGame),runningGame)==0) {
            qDebug() << "igra radi";
            char* tmp = NULL;   //if user isn't running application as administrator, then there is no need to execute function for seeking relevant IP address because current network traffic isn't in progress - so we are setting that (s)he isn't playing on any gameserver
            if (this->adminMode) {
                tmp = foundGameserverAddress(runningGame);
            }

            if (tmp == NULL && gameserverInfo != NULL) {   //user was till now on some game server and now he is not
                delete [] gameserverInfo;
                gameserverInfo = NULL;
            }
            else if (tmp != NULL && gameserverInfo == NULL) {  //user was till now in game, but wasn't on any game server
                gameserverInfo = tmp;
                tmp = NULL;
            }
            else if (tmp == NULL && gameserverInfo == NULL) {  //user still hasn't joined any game server
                sleep(10);
                continue;   //state hasn't changed - therefore we must execute other commands in this "while" block (other users already know on which game and on which gameserver are we playing)
            }
            else if (strcmp(tmp,gameserverInfo)!=0) {  //user has stayed playing same game, but he switched to another game server
                delete [] gameserverInfo;
                gameserverInfo = tmp;
                tmp = NULL;
            }
            else if (strcmp(tmp,gameserverInfo)==0) {  //if user plays on same server on which he played before 10 seconds
                delete [] tmp;
                sleep(10);
                continue;   //there is no need to send notification message if state hasn't changed
            }
            sendNotificationMessage(1,NULL,runningGame,gameserverInfo); //notifies server and other users that you have changed your gaming status
            if (tmp != NULL) {
                delete [] tmp;
            }
            sleep(10);
        }
        if (runningGame!=NULL) {       //the game is obviously not active anymore (otherwise program would be still in upper loop)
            delete [] runningGame;
            if (this->adminMode) {      //if user has not run application as administrator, then no network traffic is captured and file wasn't locked, so there is no need to unlock it
                if (gameserverInfo!=NULL) {        //if we were on some gameserver at the moment when we left that game
                    delete [] gameserverInfo;
                }
                file.close();       //we are unlocking the lock file because no game is active, so no game is generating network traffic (what means no need for tracking)
            }
            sendNotificationMessage(1,NULL,NULL,NULL);    //this is required to add if user instantly starts another game after he lefts previous one
        }
    }
}

void MainWindow::sendNotificationMessage (short tID, const char* customStatus=NULL, char* playedGameName=NULL, char* gameserverInfo=NULL) {  //notifies friends that you have started playing some game (or you stopped playing) XOR you have changed your custom status (note on exlusive or because function is separately called when user changed custom status and when his/her game status was changed
/*    if (customStatus==NULL && playedGameName==NULL && gameserverInfo==NULL) {   //if we haven't changed custom status and are not playing on any game
        if (this->currentGame == "") {     //if we haven't also played anything 10 seconds before - we don't need to notify others that we are not playing anything because they already know that
            return;
        }
    }   */

    if (customStatus==NULL) {              //if user's GAME status was changed
        if (playedGameName==NULL) {           //if user stoped playing current game
            this->currentGame = "";
        }
        else {      //if user started playing game or remained playing current game on another game server
            std::fstream file;
            tGames gameRecord;

            this->fileHandlingMutex.lock();

            file.open("gameslist.dat",std::ios::in | std::ios::binary);
            int position = binarySearchWrapper(file,playedGameName);
            file.seekg(position*sizeof(tGames));
            file.read( (char*)&gameRecord,sizeof(tGames) );
            file.close();

            this->fileHandlingMutex.unlock();

            if (gameserverInfo==NULL) {        //if user started game but isn't on any server or (s)he disconnected from current and isn't playing on any right now
                this->currentGame = QString::fromUtf8(gameRecord.fullName);
            }
            else if (gameserverInfo!=NULL) {   //if user joined some game server
                this->currentGame = QString::fromUtf8(gameRecord.fullName) + " (" + QString::fromUtf8(gameserverInfo) + ")";
            }
        }
        this->ui->currentlyPlayingTBox->setText(this->currentGame);
        this->ui->currentlyPlayingTBox->setWhatsThis(playedGameName);
    }
    else {      //if user changed his/her CUSTOM status
        this->customStatus = customStatus;
    }

    QJsonObject user;
    user["connection"] = "0026";
    user["custom_status"] = this->customStatus;
    user["current_game"] = this->currentGame;

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));

    this->socket->write(packet);
    this->socket->flush();
    qDebug() << "User status information is successfully sent!";
}

void MainWindow::on_currentStatusCBox_activated(const QString &arg1)
{
    if (QString::compare(arg1,this->customStatus)!=0) {
        this->customStatus=arg1;
        this->sendNotificationMessage(0,this->customStatus.toStdString().c_str(),NULL,NULL);
    }
}

void MainWindow::refreshGamesList () {    //refreshes Table in "My Games" tab with added paths to games (if user adds path for some game, that means that he has that game and it is added on My Games list)
    this->ui->listWidget->clear();    //deletes all previous content of that list before adding new one (otherwise, there would be many duplicates)
    std::fstream file, file2;
    tPath recordPath;
    tGames recordGame;

    this->fileHandlingMutex.lock();

    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    file2.open("gameslist.dat",std::ios::in | std::ios::binary);
    while (true) {
        file.read( (char*)&recordPath,sizeof(tPath) );
        if (file.eof()) {
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
        this->ui->listWidget->addItem(recordGame.fullName);
    }
    file.close();
    file.clear();
    file2.close();

    this->fileHandlingMutex.unlock();

}

void MainWindow::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    std::fstream file, file2;
    tPath recordPath;
    tGames recordGame;

    this->fileHandlingMutex.lock();

    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    file2.open("gameslist.dat",std::ios::in | std::ios::binary);
    while (true) {
        file.read( (char*)&recordPath,sizeof(tPath) );
        if (file.eof()) {
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
            this->fileHandlingMutex.unlock();
            this->startProgram(recordPath.processName, NULL, NULL);
            return;
        }
    }
    file.close();
    file2.close();

    this->fileHandlingMutex.unlock();

}

void MainWindow::updateSupportedGamesList (QJsonObject message, int packetSize) {
    std::fstream file;
    this->fileHandlingMutex.lock();
    file.open("gameslist.dat",std::ios::out | std::ios::binary);
    QByteArray data;    //received stringified data (\0 became \u0000 and numbers are not in numeric type) has to be converted back to
    data.setRawData( message["file"].toString().toStdString().c_str() , packetSize-2-45 );    //size of stringified file data is equal to size of whole packet -2 (what are last 2 characters in packet: "} ) -41 (there are 45 characters in packet before file data (including "file" key))
//char* ptr = new char [message["size"].toString().toInt()];
//        memcpy(ptr,data,message["size"].toString().toInt());     //data.data()   is causing program to crash
    file.write( message["file"].toString().toStdString().c_str() , message["size"].toString().toInt() );
    file.close();
    this->fileHandlingMutex.unlock();
    qDebug () << "File has been successfully downloaded!";
}

void MainWindow::on_userStatsButton_clicked()
{
    QMessageBox msgBox;
    int row = this->ui->tableWidget->currentRow();
    if (row == -1) {   //if any row isn't selected
        msgBox.setText("First select row with user for which you want get game statistics!");
        msgBox.exec();
        return;
    }
    this->requestGameActivityInfo(this->ui->tableWidget->item(row, 0)->text());
}

void MainWindow::requestGameActivityInfo(QString email)
{
    QJsonObject user;
    user["connection"] = "0027";
    user["email"] = email;

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));

    this->socket->write(packet);
    this->socket->flush();
    qDebug() << "Request for game statistics about " << user["email"].toString() << " has been successfully sent!";
}

void MainWindow::on_joinFriendButton_clicked()
{
    int row = this->ui->tableWidget->currentRow();
    QMessageBox msgBox;
    if (row == -1) {   //if any row isn't selected
        msgBox.setText("First select row with user over whom you want to join gameserver!");
        msgBox.exec();
        return;
    }
    QString gameInfo = this->ui->tableWidget->item(row,2)->text();
    if (gameInfo.isEmpty()) {
        msgBox.setText("User on which you have clicked isn't playing any game.");
        msgBox.exec();
        return;
    }
    std::string gameName = extractGameNameOnly(gameInfo).toStdString();
    if (gameName == gameInfo.toStdString()) {
        msgBox.setText("User on which you have clicked isn't playing on any gameserver");
        msgBox.exec();
        return;
    }
    std::string serverIpAndPortInsideBrackets = gameInfo.replace(0, gameName.length()+2, "").toStdString();

    std::fstream file;

    this->fileHandlingMutex.lock();

    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    tGames gameRecord;
    while (true) {
        file.read( (char*)&gameRecord,sizeof(tGames) );
        if (file.eof()) {
            file.close();
            file.clear();
            this->fileHandlingMutex.unlock();
            msgBox.setText("User is playing some game that isn't listed in your game library! Check if there's any newer version of supported games list available.");
            msgBox.exec();
            return;
        }
        if (  strcmp( gameRecord.fullName , gameName.c_str() )==0  ) {
            break;
        }
    }
    file.close();
    this->fileHandlingMutex.unlock();

    int delimiterPosition = serverIpAndPortInsideBrackets.find_first_of(':');
    startProgram( gameRecord.processName , serverIpAndPortInsideBrackets.substr(0, delimiterPosition).c_str() , serverIpAndPortInsideBrackets.substr(delimiterPosition+1,serverIpAndPortInsideBrackets.length()-delimiterPosition-2).c_str() );
}

void MainWindow::on_actionMyStats_triggered()
{
    this->requestGameActivityInfo(this->username);
}

void MainWindow::on_instantChatButton_clicked()
{
    short activeRow = this->ui->tableWidget->currentRow();
    if (activeRow == -1) {
        QMessageBox msgBox;
        msgBox.setText("First select row with user with which you want to chat!");
        msgBox.exec();
        return;
    }
    QString username = this->ui->tableWidget->item(activeRow,0)->text();
    if (this->username == username) {
        QMessageBox msgbox;
        msgbox.setText("Get a friend mate!");
        msgbox.exec();
        return;
    }
    if (this->ui->tableWidget->item(activeRow,1)->text() == "Offline") {
        QMessageBox msgbox;
        msgbox.setText("You can't chat with offline user.");
        msgbox.exec();
        return;
    }
    ChatWindow* chatbox = new ChatWindow(this->socket, username, this);
    this->privateChatMap.insert(username, chatbox);
    chatbox->show();
}

void MainWindow::on_actionDisconnect_triggered()
{
    this->close();
    this->socket->close();
    QApplication::exit(1);      //restart application (show login screen again)
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
    this->socket->close();
    QApplication::exit(0);      //shut down application
}

void MainWindow::on_tableWidget_cellDoubleClicked(int row, int column)
{
    this->on_instantChatButton_clicked();
}

void MainWindow::onTcpMessageReceived() {
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    disconnect(socket, SIGNAL(readyRead()), this, SLOT(onTcpMessageReceived()));
    int curlyBracketsState = 0;
    std::vector<int> ranges;
    int rangeSize = 0;
    bool first = true;
    long long packetSize = 0;
    do {
        QByteArray fragment;
        if (!first) {
            this->socket->waitForReadyRead();
        }
        fragment = this->socket->readAll();
        packet += fragment;

        int fragmentSize = fragment.size();
        byte numOfConsecutiveEscapeChars = 0;
        char currChar;
        for (int i=0; i<fragmentSize; i++) {
            currChar = fragment.at(i);
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
                            ranges.push_back(packetSize + i + 1);
                            rangeSize++;
                        }
                    }
                }
                numOfConsecutiveEscapeChars = 0;
            }
        }
        first = false;
        packetSize += fragmentSize;
    } while (curlyBracketsState != 0);
    connect(socket, SIGNAL(readyRead()), this, SLOT(onTcpMessageReceived()));
    int lowerLimit = 0;
    for (int i=0; i<rangeSize; i++) {
        int upperLimit = ranges.at(i);

        document = QJsonDocument::fromJson(packet.mid(lowerLimit, upperLimit - lowerLimit).constData());
        object = document.object();

        QString messageCode = object["connection"].toString();
        switch (messageCode.toInt()) {
            case 5:
                this->addFriendBox->statusLabel->setText("Sorry, that user does not seem to exist in our database!");
                this->addFriendBox->statusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
                break;
            case 9:
                this->addFriendBox->statusLabel->setText("Friend request sent!");
                this->addFriendBox->statusLabel->setStyleSheet("QLabel { background-color : white; color : green; }");
                break;
            case 10:
                this->addFriendBox->statusLabel->setText("That user is already your friend!");
                this->addFriendBox->statusLabel->setStyleSheet("QLabel { background-color : white; color : yellow; }");
                break;
            case 12:
                this->getFriendsList(object);
                break;
            case 13:
                this->addFriendBox->statusLabel->setText("Don't be silly!");
                this->addFriendBox->statusLabel->setStyleSheet("QLabel { background-color : white; color : yellow; }");
                break;
            case 14:
                this->addFriendBox->statusLabel->setText("Your previous request is still pending!");
                this->addFriendBox->statusLabel->setStyleSheet("QLabel { background-color : white; color : yellow; }");
                break;
            case 16:
                this->refreshFriendsList(object);
                break;
            case 17:
                this->processFriendRequest(object["email"].toString());
                break;
            case 19:
                qDebug () << "You already have the latest version of list of supported games.";
                this->autoDetectGames();

                this->refreshGamesList();     //refreshes table in "My Games" tab with games that user owns (for which (s)he defined path)

                if (!this->initialGamesListCheckingDone) {
                    this->initialGamesListCheckingDone = true;
                }
                else {
                    QMessageBox msgBox;
                    msgBox.setText("Current list of supported games is up to date!");
                    msgBox.exec();
                }
                break;
            case 20:
                this->updateSupportedGamesList(object, packet.size());
                this->autoDetectGames();

                this->refreshGamesList();     //refreshes table in "My Games" tab with games that user owns (for which (s)he defined path)

                if (!this->initialGamesListCheckingDone) {
                    this->initialGamesListCheckingDone = true;
                }
                else {
                    this->autoDetectGames();
                    this->refreshGamesList();
                    this->gameLibWin->fillTable();
                    QMessageBox msgBox;
                    msgBox.setText("Newer version of list of supported games has been found and received!");
                    msgBox.exec();
                }
                break;
            case 22:
            case 23:
                this->handleNewChatMessage(object);
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

void MainWindow::handleNewChatMessage(QJsonObject message) {
    if (message["isprivate"].toBool()) {
        QString sender = message["chatid"].toString();
        ChatWindow* chatbox;
        if (chatbox = this->privateChatMap[sender]) {
            chatbox->update(message);
            chatbox->show();
            return;
        }
        chatbox = new ChatWindow(message, this->socket, sender, this);
        this->privateChatMap.insert(sender, chatbox);
        chatbox->show();
    }
    else {
        QString chatRoomId = message["chatid"].toString();
        ChatWindow* chatbox;
        if (chatbox = this->groupChatMap[chatRoomId]) {
            chatbox->update(message);
            chatbox->show();
            return;
        }
        chatbox = new ChatWindow(message, this->socket, chatRoomId, this, true);
        this->groupChatMap.insert(chatRoomId, chatbox);
        chatbox->show();
    }
    this->snd->setObjectName("message");
}

void MainWindow::refreshFriendsList(QJsonObject message) {
    int numRows = this->ui->tableWidget->rowCount();
    QString username = message["email"].toString();
    QString newCustomStatus = message["custom_status"].toString();
    QString newGameStatus = message["current_game"].toString();
    if (this->addFriendBox != NULL && this->addFriendBox->emailLineEdit->text() == username) {
        this->addFriendBox->statusLabel->setText("Specified user is now your friend!");
        this->addFriendBox->statusLabel->setStyleSheet("QLabel { background-color : white; color : green; }");
    }
    for(int i=0; i < numRows; i++) {
        if (this->ui->tableWidget->item(i, 0)->text() == username) {
            ChatWindow* chatbox = this->privateChatMap[username];
            if (chatbox != NULL) {
                QString oldCustomStatus = this->ui->tableWidget->item(i, 1)->text();
                QString oldGameName = extractGameNameOnly(this->ui->tableWidget->item(i, 2)->text());
                QString newGameName = extractGameNameOnly(newGameStatus);
                if (oldCustomStatus != newCustomStatus) {
                    if (oldCustomStatus == "Offline") {
                        chatbox->update(username + " is now online!");
                    }
                    else if (newCustomStatus == "Offline") {
                        chatbox->update(username + " is now offline..");
                    }
                    else {
                        chatbox->update(((QString)"%1 has changed their status from \"%2\" to \"%3\"").arg(username, oldCustomStatus, newCustomStatus));
                    }
                }
                else if (oldGameName != newGameName) {
                    if (oldGameName.isEmpty()) {
                        chatbox->update(((QString)"%1 is now playing \"%2\"").arg(username, newGameName));
                    }
                    else {
                        chatbox->update(((QString)"%1 has stopped playing \"%2\"").arg(username, oldGameName));
                    }
                }
            }
            this->ui->tableWidget->item(i, 1)->setText(newCustomStatus);
            this->ui->tableWidget->item(i, 2)->setText(newGameStatus);
            return;
        }
    }
    this->ui->tableWidget->insertRow(numRows);
    this->ui->tableWidget->setItem(numRows, 0, new QTableWidgetItem(username));
    this->ui->tableWidget->setItem(numRows, 1, new QTableWidgetItem(newCustomStatus));
    this->ui->tableWidget->setItem(numRows, 2, new QTableWidgetItem(newGameStatus));
}

void MainWindow::showGameStats(QJsonObject object) {
    QJsonValue value = object.value("stats");
    QJsonArray array = value.toArray();

    QMessageBox msgBox;
    QString email = object.value("email").toString();
    if (array.count() == 0) {
        msgBox.setText("Username: " + email + "\r\n\r\nThis user hasn't played any game yet!");
    }
    else {
        QString statslist = "<caption>Username: " + email + "<br/></caption><thead><tr><th>Game name</th><th align=\"right\">Time played</th></tr></thead><tbody>";
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
            statslist += "<tr><td>" + sortedArray[i].toObject().value("game").toString() + "</td><td align=\"right\" valign=\"middle\">" + convertSecondsToHmsFormat(sortedArray[i].toObject().value("time_played").toDouble()) + "</td></tr>";
        }
		msgBox.setText("<table border=\"2\" cellspacing=\"0\">" + statslist + "</tbody></table>");
    }
    msgBox.setWindowTitle( "Games statistics for user " + email );
    msgBox.exec();
}

void MainWindow::checkIfNewerGamesListExist() {
    int size;
    std::fstream file;

    this->fileHandlingMutex.lock();
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
    this->fileHandlingMutex.unlock();

    QJsonObject user;
    user["connection"] = "0018";
    user["size"] = size;
    user["datetime"] = datetime.str().c_str();  //if there is no file yet, then datetime of last modification is undefined and very old date is sent

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));

    this->socket->write(packet);
    this->socket->flush();
    qDebug() << "Request for update info about gameslist.dat has been successfully sent!";
}

void MainWindow::startProgram (const char* progName, const char* ip, const char* port) {    //start game which server flagged as supported (in gameslist.dat file) and which path is defined (in gamepath.dat file)
    std::stringstream command;
    std::fstream file;
    tGames gameRecord;

    this->fileHandlingMutex.lock();
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    file.seekg( binarySearchWrapper(file,progName)*sizeof(tGames) , std::ios::beg );
    file.read( (char*)&gameRecord,sizeof(tGames) );
    file.close();

    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    tPath pathRecord;
    bool pathNotDefined=true;
    while (true) {
        file.read( (char*)&pathRecord,sizeof(tPath) );
        if (file.eof()) {
            pathNotDefined=false;
            break;
        }
        if (strcmp(pathRecord.processName,progName)==0) {
            break;
        }
    }

    file.close();
    this->fileHandlingMutex.unlock();
    if (!pathNotDefined || strcmp(pathRecord.path,"")==0) {   //record about some game can be stored locally if user removes game from his list of games (if he set game path to nullString)
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
    command << path.c_str() << "/" << progName;
#endif

    if (ip!=NULL && strcmp(gameRecord.multiplayerCommandLineArguments,"\0")!=0) {   //if in this function are passed ip address and port of some remote server and if there exists a way to join specific gameserver in game directly via command line
        launchArguments = launchArguments.replace("%%exe%%", progName);
        launchArguments = launchArguments.replace("%%ip%%", ip);
        launchArguments = launchArguments.replace("%%port%%", port);
        command << " " << launchArguments.toStdString().c_str();
    }
    else {
        command << progName;
    }
    command << " " << pathRecord.customExecutableParameters;

    system(command.str().c_str());  //executing
}
