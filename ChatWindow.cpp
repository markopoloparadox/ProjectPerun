#include "ChatWindow.h"
#include "ui_ChatWindow.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTableWidget>
#include <QMessageBox>
#include <QStringList>

ChatWindow::ChatWindow(QJsonObject object, QTcpSocket *socket, QString name, QWidget *parent, bool isGroupChat) :
    ChatWindow(socket, name, parent, isGroupChat)
{
    this->ui->textBrowser->append(object["msg"].toString());

    if (isGroupChat) {
        QString users = "";

        for (auto i : object["userlist"].toArray()) {
            users += i.toString() + '\n';
        }
        this->ui->userListTextBrowser->setText(users);
    }
}

ChatWindow::ChatWindow(QTcpSocket *socket, QString name, QWidget *parent, bool isGroupChat) :
    QMainWindow(parent),
    ui(new Ui::ChatWindow),
    isGroupChat(isGroupChat),
    chatId(name),
    socket(socket)
{
    this->ui->setupUi(this);

    if (!isGroupChat) {
        this->ui->userListTextBrowser->hide();
        this->ui->exitButton->hide();
        this->setWindowTitle(name);
    }
    else {
        this->setWindowTitle("Group chat #" + name);
    }
    this->ui->enterLineEdit->setFocus();

    connect(this->ui->enterButton, SIGNAL(clicked(bool)), this, SLOT(sendMessage()));
    connect(this->ui->exitButton, SIGNAL(clicked(bool)), this, SLOT(leaveChatRoom()));
    connect(this->ui->addUserToChatButton, SIGNAL(clicked(bool)), this, SLOT(addFriendInChatRoom()));
    connect(this->ui->enterLineEdit, SIGNAL(returnPressed()), this, SLOT(sendMessage()));
}

ChatWindow::~ChatWindow() {
    delete ui;
}

void ChatWindow::update(QJsonObject object){
    if (object["connection"] == "0022") {
        QString users = "";
        for (auto i : object["userlist"].toArray()) {
            users += i.toString() + '\n';
        }
        this->ui->userListTextBrowser->setText(users);
    }

    this->ui->textBrowser->append(object["msg"].toString());

    if (!this->isActiveWindow()) {
        ((MainWindow*)this->parent())->snd->setObjectName("message");
    }
}

void ChatWindow::update(QString message){
    this->ui->textBrowser->append(message);

    if (this->isVisible() && !this->isActiveWindow()) {
        ((MainWindow*)this->parent())->snd->setObjectName("message");
    }
}

void ChatWindow::sendMessage(){
    if (!this->ui->enterLineEdit->text().isEmpty()) {
        QJsonObject object;
        QJsonDocument document;
        QByteArray packet;

        object["connection"] = "0023";
        object["chatid"] = this->chatId;
        object["isprivate"] = !this->isGroupChat;
        object["msg"] = this->ui->enterLineEdit->text();


        document.setObject(object);
        packet = document.toJson(QJsonDocument::Compact);

        this->socket->write(packet);
        this->socket->flush();
        this->ui->enterLineEdit->clear();
    }
}

void ChatWindow::leaveChatRoom() {
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0025";
    object["chatid"] = this->chatId;
    document.setObject(object);
    packet = document.toJson(QJsonDocument::Compact);

    this->socket->write(packet);
    this->socket->flush();

    ((MainWindow*)this->parent())->groupChatMap.remove(this->chatId);
    this->destroy();
}

void ChatWindow::addFriendInChatRoom(){
    QString username = this->ui->enterLineEdit->text();
    if (username.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText("In order to add friend to this chat, you have to enter their username in message field!");
        msgBox.exec();
    }
    else {
        QRegularExpression delimiter("\\s*,\\s*|\\s+");
        QStringList listOfCurrentUsersInChat;
        if (this->isGroupChat) {
            listOfCurrentUsersInChat = this->ui->userListTextBrowser->toPlainText().split('\n', QString::SplitBehavior::SkipEmptyParts);
        }
        else {
            listOfCurrentUsersInChat += { this->chatId, ((MainWindow*)this->parent())->username };
        }
        QStringList usernameList = this->ui->enterLineEdit->text().trimmed().split(delimiter, QString::SplitBehavior::SkipEmptyParts);
        QJsonArray usernameJsonArray;
        QTableWidget* friendsListTable = ((MainWindow*)this->parent())->ui->tableWidget;
        int numRows = friendsListTable->rowCount();
        int numOfUsernames = usernameList.length();
        for (int i=0; i<numOfUsernames; i++) {
            QString username = usernameList.at(i);
            bool notInFriendList = true;
            bool friendOffline = true;
            if (listOfCurrentUsersInChat.contains(username)) {
                if (this->isGroupChat) {
                    QMessageBox msgBox;
                    msgBox.setText("User(s) you've tried to add is/are already in chatroom");
                    msgBox.exec();
                    return;
                }
                else {
                    continue;
                }
            }
            for (int i=0; i < numRows; i++) {
                if (friendsListTable->item(i, 0)->text() == username) {
                    notInFriendList = false;
                    if (friendsListTable->item(i, 1)->text() != "Offline") {
                        friendOffline = false;
                    }
                    break;
                }
            }
            if (notInFriendList) {
                QMessageBox msgbox;
                msgbox.setText("User(s) you've tried to add is/are not in your friends list!");
                msgbox.exec();
                return;
            }
            else if (friendOffline) {
                QMessageBox msgbox;
                msgbox.setText("User(s) you've tried to add is/are currently offline!");
                msgbox.exec();
                return;
            }
            usernameJsonArray.push_back(username);
        }
        if (usernameJsonArray.size() == 0) {
            QMessageBox msgBox;
            msgBox.setText("Except you and current interlocutor, there has to be at least one more participant!");
            msgBox.exec();
            return;
        }

        QJsonObject object;
        QJsonDocument document;
        QByteArray packet;

        if (this->isGroupChat) {
            object["connection"] = "0024";
            object["chatid"] = this->chatId;
            object["userlist"] = usernameJsonArray;
        }
        else {
            object["connection"] = "0021";
            usernameJsonArray.push_back(this->chatId);
            object["userlist"] = usernameJsonArray;
        }
        document.setObject(object);
        packet = document.toJson(QJsonDocument::Compact);

        this->socket->write(packet);
        this->socket->flush();
        this->ui->enterLineEdit->clear();
    }
}
