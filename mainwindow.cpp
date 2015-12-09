#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QTcpSocket *socket, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_Socket = socket;
    on_RefreshFriendsButton_clicked();
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(on_RefreshFriendsButton_clicked()));
    timer->start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
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
    user["connection"] = "0008";
    user["email"] = username;

    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));
    m_Socket->write(packet);
    m_Socket->flush();



    m_Socket->waitForReadyRead();
    packet = m_Socket->readAll();
    object = QJsonDocument::fromJson(packet.constData());
    user = object.object();
}
