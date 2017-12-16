#ifndef CHATBOX_H
#define CHATBOX_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUdpSocket>
#include <QJsonArray>

namespace Ui {
class ChatBox;
}

class ChatBox : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatBox(QJsonObject object, QTcpSocket *socket, QString name, QWidget *parent = 0, bool isGroupChat = false);
    ChatBox::ChatBox(QTcpSocket *socket, QString name, QWidget *parent, bool isGroupChat = false);
    ~ChatBox();
    QString m_ChatId;
    void Update(QJsonObject object);

public slots:
    void SendMsg();
    void CloseMsg();
    void AddFriend();

private:
    Ui::ChatBox *ui;
    QTcpSocket *m_Socket;
    bool isGroupChat;
};

#endif // CHATBOX_H
