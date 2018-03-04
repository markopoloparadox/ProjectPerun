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
class ChatWindow;
}

class ChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatWindow(QJsonObject object, QTcpSocket *socket, QString name, QWidget *parent = 0, bool isGroupChat = false);
    ChatWindow(QTcpSocket *socket, QString name, QWidget *parent, bool isGroupChat = false);
    ~ChatWindow();
    QString chatId;
    void update(QJsonObject object);
    void update(QString message);

public slots:
    void sendMessage();
    void leaveChatRoom();
    void addFriendInChatRoom();

private:
    Ui::ChatWindow *ui;
    QTcpSocket *socket;
    bool isGroupChat;
};

#endif // CHATBOX_H
