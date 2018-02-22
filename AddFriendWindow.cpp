#include "AddFriendWindow.h"
#include "MainWindow.h"

AddFriendWindow::AddFriendWindow(QTcpSocket *socket, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AddFriendWindow),
    socket(socket)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog);
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    connect(this->ui->cancelButton, SIGNAL(clicked(bool)), this, SLOT(close()));

}

AddFriendWindow::~AddFriendWindow() {
    ((MainWindow*)this->parent())->addFriendBox = NULL;
    delete this->ui;
}

void AddFriendWindow::on_addButton_clicked() {
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;
    QString username;

    username = this->ui->emailLineEdit->text();
    if(username.isEmpty()) {
        this->ui->statusLabel->setText("Please enter all the information!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    object["connection"] = "0008";
    object["email"] = username;
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    this->socket->write(packet);
    this->socket->flush();
}
