#include "chatbox.h"
#include "ui_chatbox.h"

ChatBox::ChatBox(QTcpSocket* socket, QString ip, qint16 port, QString myName, QString hisName, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatBox)
{
    ui->setupUi(this);
    if(socket == nullptr) {
        m_Socket = new QTcpSocket;
        m_Socket->connectToHost(ip, port);
        m_Socket->waitForConnected();
        if(m_Socket->state() == QAbstractSocket::UnconnectedState)
            this->close();
        m_IncClientName = myName;
        m_OutClientName = hisName;
        qDebug() << myName <<" " << hisName;

        QJsonObject packet;
        packet["connection"] = "0018";
        packet["icName"] = m_IncClientName;
        packet["ocName"] = m_OutClientName;
        ui->TextBrowser->append(m_IncClientName);

        QJsonDocument docPacket(packet);
        QByteArray rawPacket = (docPacket.toJson(QJsonDocument::Compact));
        m_Socket->write(rawPacket);
        m_Socket->flush();

    } else {
        m_Socket = socket;
    }
    connect(m_Socket, SIGNAL(readyRead()), this, SLOT(Listen()));
}

ChatBox::~ChatBox() {
    delete ui;
    delete m_Socket;
}

void ChatBox::Listen() {
    QByteArray rawPacket;
    rawPacket = m_Socket->readAll();
    QJsonDocument docPacket;
    docPacket = QJsonDocument::fromJson(rawPacket.constData());
    QJsonObject packet;
    packet = docPacket.object();

    if(packet["connection"] == "0017") {
        ui->TextBrowser->append(m_OutClientName + ":" + packet["msg"].toString());
    } else if (packet["connection"] == "0018") {
        m_IncClientName = packet["ocName"].toString();
        m_OutClientName = packet["icName"].toString();
    }

}

void ChatBox::on_EnterButton_clicked() {
    QJsonObject packet;
    packet["connection"] = "0017";
    packet["msg"] = ui->EnterLineEdit->text();
    ui->TextBrowser->append(m_IncClientName + ":" + ui->EnterLineEdit->text());

    QJsonDocument docPacket(packet);
    QByteArray rawPacket = (docPacket.toJson(QJsonDocument::Compact));
    m_Socket->write(rawPacket);
    m_Socket->flush();
}
