#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gamelibrary.h"
#include <fstream>
#include <cstring>
#include <QString>

MainWindow::MainWindow(QTcpSocket *socket, qint16 port, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_Socket = socket;
    ui->currentStatusCBox->addItem("Online");
    ui->currentStatusCBox->addItem("Away From Keyboard");
    ui->currentStatusCBox->addItem("Busy");
    this->custom_status="Online";
    this->current_game="";
    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
//    ui->actionLaunch = new QActionGroup(this);
    on_RefreshFriendsButton_clicked();
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(on_RefreshFriendsButton_clicked()));
    timer->start(1000);
    m_Server = new QTcpServer();
    connect(m_Server, SIGNAL(newConnection()), this, SLOT(NewConnection()));
    m_Server->listen(QHostAddress::Any, port);
    std::fstream file;
    file.open("gamepath.dat",std::ios::app); //create file if it does not exists; otherwise do nothing (it won't be changed)
    file.close();
    //potrebno je i zaprimiti posljednju verziju gameslist.dat datoteke!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    std::thread *listener_thread = new std::thread (outer_function);
}

MainWindow::~MainWindow()
{
    listener_thread->join();
    delete ui;
}

void MainWindow::NewConnection() {
    QTcpSocket* socket = m_Server->nextPendingConnection();
    ChatBox* cBox = new ChatBox(socket);
    cBox->show();
}

void MainWindow::on_AddFriendButton_clicked()
{
    AddFriend* addfrindbox = new AddFriend(m_Socket);
    addfrindbox->show();

}

void MainWindow::on_RefreshFriendsButton_clicked()
{
    QJsonObject user;
    user["connection"] = "0011";

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));
    m_Socket->write(packet);
    m_Socket->flush();

    m_Socket->waitForReadyRead();
    packet = m_Socket->readAll();
    object = QJsonDocument::fromJson(packet.constData());
    user = object.object();

    QJsonValue value = user.value("friends");
    QJsonArray array = value.toArray();

    while(ui->tableWidget->rowCount() < array.count())
        ui->tableWidget->insertRow( ui->tableWidget->rowCount());

    for(int i = 0; i < array.count(); ++i) {
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem(array[i].toObject().value("email").toString()));
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem(array[i].toObject().value("status").toString()));
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem(array[i].toObject().value("currgame").toString()));
    }
}


void MainWindow::on_ChatButton_clicked()
{
    QString username;
    username = ui->EmailLineEdit->text();

    if(username == "") {
        ui->StatusLabel->setText("Please enter all the information!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    QJsonObject user;
    user["connection"] = "0014";
    user["email"] = username;

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));
    m_Socket->write(packet);
    m_Socket->flush();



    m_Socket->waitForReadyRead();
    packet = m_Socket->readAll();
    object = QJsonDocument::fromJson(packet.constData());
    user = object.object();

    if(user["connection"] == "0015") {
        ui->StatusLabel->setText("User is not online!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return ;
    } else if (user["connection"] == "0016") {
        ChatBox* cBox = new ChatBox(nullptr, user["ip"].toString(), user["port"].toInt(),  user["clname"].toString(), ui->EmailLineEdit->text());
        cBox->show();

    }

}
void MainWindow::on_actionConfigure_game_library_triggered()
{
    gamelibrary* gameLibWin = new gamelibrary(this);
    gameLibWin->show();
}

void MainWindow::update_state()
{
    game_detection();
}

void outer_function () {    //my compiler does not not allow that threaded function is class member (i.e. method), so this is workaround - people say that that it compiler's bug
    MainWindow::update_state();
}

void MainWindow::on_currentStatusCBox_activated(const QString &arg1)
{
    if (QString::compare(arg1,this->custom_status)!=0) {
        this->custom_status=arg1;
        this->setOverallStatus();
    }
}

void MainWindow::send_notification_message (char* played_game_name,char* gameserver_info=NULL) {
    QString game;
    if (played_game_name==NULL) {
        game="";
    }
    else if (gameserver_info==NULL) {
        game=QString::fromUtf8(played_game_name);
    }
    else if (gameserver_info!=NULL) {
        game=QString::fromUtf8(played_game_name) + "(" + QString::fromUtf8(gameserver_info) + ")";
    }

    //if (QString::compare(this->current_game,game)!=0) {
        //this->current_game = game;
        //this->setOverallStatus();
    //}
}

void MainWindow::setOverallStatus() {
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
}


void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    if (index==2) {
        ui->listWidget->clear();
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

void MainWindow::on_refreshButton_clicked()
{
    qDebug() << "abc";
}
