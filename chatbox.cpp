#include "chatbox.h"
#include "ui_chatbox.h"

ChatBox::ChatBox(QJsonObject object, QTcpSocket *socket, QString name, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatBox)
{
    ui->setupUi(this);
    ui->TextBrowser->append(object["msg"].toString());

    this->setWindowTitle(name);
    this->m_Socket = socket;
    this->m_UsVec = object["userlist"].toArray();
    this->m_ChatId = object["chatid"].toInt();
    this->m_Users = "";
    ui->EnterLineEdit->setFocus();

    for(int i = 0; i < m_UsVec.size(); ++i)
        this->m_Users += (m_UsVec[i].toString()+ " ");

    ui->UserListTextBrowser->setText(m_Users);

    connect(ui->EnterButton, SIGNAL(clicked(bool)), this, SLOT(SendMsg()));
    connect(ui->ExitButton, SIGNAL(clicked(bool)), this, SLOT(CloseMsg()));
    connect(ui->AddUserToChatButton, SIGNAL(clicked(bool)), this, SLOT(AddFriend()));
    connect(ui->EnterLineEdit, SIGNAL(returnPressed()), this, SLOT(SendMsg()));

}

ChatBox::~ChatBox() {
    delete ui;
}

void ChatBox::Update(QJsonObject object){
    m_UsVec = object["userlist"].toArray();
    m_Users = "";

    for(int i = 0; i < m_UsVec.size(); ++i)
        m_Users += (m_UsVec[i].toString()+ " ");

    ui->UserListTextBrowser->setText(m_Users);
    ui->TextBrowser->append(object["msg"].toString());

}

void ChatBox::SendMsg(){
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0023";
    object["chatid"] = m_ChatId;
    object["userlist"] = m_UsVec;
    object["msg"] = ui->EnterLineEdit->text();
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(1000);
    ui->EnterLineEdit->clear();
}

void ChatBox::CloseMsg() {
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0025";
    object["chatid"] = m_ChatId;
    object["userlist"] = m_UsVec;
    object["msg"] = "Has left...";
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(1000);

    this->destroy();
}

void ChatBox::AddFriend(){
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0024";
    object["chatid"] = m_ChatId;
    object["userlist"] = m_UsVec;
    object["msg"] = "";
    object["username"] = ui->EnterLineEdit->text();
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(1000);
    m_Socket->waitForReadyRead(3000);

    packet = m_Socket->readAll();
    document = QJsonDocument::fromJson(packet.constData());
    object = document.object();

    if(object["connection"] == "0015") {
        ui->EnterLineEdit->setText("Erroro");
    }
}
