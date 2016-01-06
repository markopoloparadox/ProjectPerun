#include "addfriend.h"
#include "ui_addfriend.h"

AddFriend::AddFriend(QTcpSocket *socket, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AddFriend)
{
    ui->setupUi(this);
    this->m_Socket = socket;

    connect(ui->CancelButton, SIGNAL(clicked(bool)), this, SLOT(close()));

}

AddFriend::~AddFriend() {
    delete ui;
}

void AddFriend::on_AddButton_clicked() {
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;
    QString username;

    username = ui->EmailLineEdit->text();
    if(username == "") {
        ui->StatusLabel->setText("Please enter all the information!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    object["connection"] = "0008";
    object["email"] = username;
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->flush();
    m_Socket->waitForReadyRead();

    packet = m_Socket->readAll();
    document = QJsonDocument::fromJson(packet.constData());
    object = document.object();


    if(object["connection"] == "0005") {
        ui->StatusLabel->setText("Sorry, that user does not seem to exist in our database!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
        return;
    }
    if(object["connection"] == "0009") {
        ui->StatusLabel->setText("Done!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    if(object["connection"] == "0010") {
        ui->StatusLabel->setText("That user is already your friend!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    if(object["connection"] == "0013") {
        ui->StatusLabel->setText("Don't be silly!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }
}
