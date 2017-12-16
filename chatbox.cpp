#include "chatbox.h"
#include "ui_chatbox.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTableWidget>
#include <QMessageBox>
#include <QStringList>

ChatBox::ChatBox(QJsonObject object, QTcpSocket *socket, QString name, QWidget *parent, bool isGroupChat) :
    ChatBox(socket, name, parent, isGroupChat)
{
    ui->TextBrowser->append(object["msg"].toString());

    if (isGroupChat) {
        QString users = "";

        for (auto i : object["userlist"].toArray()) {
            users += i.toString() + '\n';
        }
        ui->UserListTextBrowser->setText(users);
    }
}

ChatBox::ChatBox(QTcpSocket *socket, QString name, QWidget *parent, bool isGroupChat) :
    QMainWindow(parent),
    ui(new Ui::ChatBox),
    isGroupChat(isGroupChat),
    m_ChatId(name),
    m_Socket(socket)
{
    ui->setupUi(this);

    if (!isGroupChat) {
        ui->UserListTextBrowser->hide();
        ui->ExitButton->hide();
        this->setWindowTitle(name);
    }
    else {
        this->setWindowTitle("Group chat #" + name);
    }
    ui->EnterLineEdit->setFocus();

    connect(ui->EnterButton, SIGNAL(clicked(bool)), this, SLOT(SendMsg()));
    connect(ui->ExitButton, SIGNAL(clicked(bool)), this, SLOT(CloseMsg()));
    connect(ui->AddUserToChatButton, SIGNAL(clicked(bool)), this, SLOT(AddFriend()));
    connect(ui->EnterLineEdit, SIGNAL(returnPressed()), this, SLOT(SendMsg()));
}

ChatBox::~ChatBox() {
    delete ui;
}

void ChatBox::Update(QJsonObject object){
    if (object["connection"] == "0022") {
        QString users = "";
        for (auto i : object["userlist"].toArray()) {
            users += i.toString() + '\n';
        }
        ui->UserListTextBrowser->setText(users);
    }

    ui->TextBrowser->append(object["msg"].toString());

    if (!this->isActiveWindow()) {
        ((MainWindow*)this->parent())->snd->setObjectName("message");
    }
}

void ChatBox::SendMsg(){
    if (!ui->EnterLineEdit->text().isEmpty()) {
        QJsonObject object;
        QJsonDocument document;
        QByteArray packet;

        object["connection"] = "0023";
        object["chatid"] = m_ChatId;
        object["isprivate"] = !isGroupChat;
        object["msg"] = ui->EnterLineEdit->text();


        document.setObject(object);
        packet = (document.toJson(QJsonDocument::Compact));

        m_Socket->write(packet);
        m_Socket->flush();
        ui->EnterLineEdit->clear();
    }
}

void ChatBox::CloseMsg() {
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0025";
    object["chatid"] = m_ChatId;
    document.setObject(object);
    packet = (document.toJson(QJsonDocument::Compact));

    m_Socket->write(packet);
    m_Socket->flush();

    this->destroy();
}

void ChatBox::AddFriend(){
    QString username = ui->EnterLineEdit->text();
    if (username.isEmpty()) {
        QMessageBox msgbox;
        msgbox.setText("In order to add friend to this chat, you have to enter their username in message field!");
        msgbox.exec();
    }
    else {
        QRegularExpression delimiter("\\S*,\\S*");
        QStringList listOfCurrentUsersInChat;
        if (isGroupChat) {
            listOfCurrentUsersInChat = ui->UserListTextBrowser->toPlainText().split(delimiter, QString::SplitBehavior::SkipEmptyParts);
            qDebug() << ui->UserListTextBrowser->toPlainText();
        }
        QStringList usernameList = ui->EnterLineEdit->text().trimmed().split(delimiter, QString::SplitBehavior::SkipEmptyParts);
        QJsonArray usernameJsonArray;
        QTableWidget* friendsListTable = ((MainWindow*)this->parent())->ui->tableWidget;
        int numRows = friendsListTable->rowCount();
        int numOfUsernames = usernameList.length();
        for (int i=0; i<numOfUsernames; i++) {
            QString username = usernameList.at(i);
            bool notInFriendList = true;
            bool friendOffline = true;
            if (isGroupChat) {
                if (listOfCurrentUsersInChat.contains(username)) {
                    QMessageBox msgbox;
                    msgbox.setText("User(s) you've tried to add is/are already in chatroom");
                    msgbox.exec();
                    return;
                }
            }
            else {
                if (username == m_ChatId || username == ((MainWindow*)this->parent())->m_Name) {
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
            QMessageBox msgbox;
            msgbox.setText("Except you and current interlocutor, there has to be at least one more participant!");
            msgbox.exec();
            return;
        }

        QJsonObject object;
        QJsonDocument document;
        QByteArray packet;

        if (isGroupChat) {
            object["connection"] = "0024";
            object["chatid"] = m_ChatId;
            object["userlist"] = usernameJsonArray;
        }
        else {
            object["connection"] = "0021";
            usernameJsonArray.push_back(m_ChatId);
            object["userlist"] = usernameJsonArray;
        }
        document.setObject(object);
        packet = (document.toJson(QJsonDocument::Compact));

        m_Socket->write(packet);
        m_Socket->flush();
        ui->EnterLineEdit->clear();
    }
}
