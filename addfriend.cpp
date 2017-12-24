#include "addfriend.h"
#include "ui_addfriend.h"
#include "mainwindow.h"

AddFriend::AddFriend(QTcpSocket *socket, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AddFriend)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog);
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    this->m_Socket = socket;

    connect(ui->CancelButton, SIGNAL(clicked(bool)), this, SLOT(close()));

}

AddFriend::~AddFriend() {
    ((MainWindow*)this->parent())->addfriendbox = NULL;
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
}
