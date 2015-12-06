#include "addfriend.h"
#include "ui_addfriend.h"

AddFriend::AddFriend(QTcpSocket *socket, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AddFriend)
{
    ui->setupUi(this);
    m_Socket = socket;
}

AddFriend::~AddFriend()
{
    delete ui;
}

void AddFriend::on_AddButton_clicked()
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


    if(user["connection"] == "0005") {
        ui->StatusLabel->setText("Sorry, that user does not seem to exist in our database!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
        return;
    }
    if(user["connection"] == "0009") {
        ui->StatusLabel->setText("Done!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    if(user["connection"] == "0010") {
        ui->StatusLabel->setText("That user is already your friend!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    if(user["connection"] == "0013") {
        ui->StatusLabel->setText("Don't be silly!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }
}

void AddFriend::on_pushButton_2_clicked()
{
    this->close();
}
