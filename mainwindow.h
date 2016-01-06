#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QUdpSocket>
#include <thread>
#include <vector>
#include <QListWidgetItem>
#include "addfriend.h"
#include "chatbox.h"
#include "game_detection.h"
#include "launch_game.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QTcpSocket* socket, qint16 port, bool aMode, QString name, QWidget *parent = 0);
    ~MainWindow();
    void check_game_status();
    void refresh_games_list();
    short update_supported_games_list ();

public slots:

private slots:
    void on_AddFriendButton_clicked();

    void on_RefreshFriendsButton_clicked();

    void on_ChatButton_clicked();

    void CheckForMsg();

    void on_actionConfigure_game_library_triggered();

    void on_currentStatusCBox_activated(const QString &arg1);

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

private:
    QTcpSocket* m_Socket;
    QUdpSocket* m_UDPSocket;
    QString custom_status;  //defines custom status message which can user set and other can see (AFK, Selling piglets, Looking for match 2 vs 2)
    QString current_game;   //defines game which user currently plays - empty string if none is played
    QString m_Name;
    std::vector<ChatBox*> m_Chatvec;
    std::thread *listener_thread;
    qint16 m_Port;
    Ui::MainWindow *ui;
    bool adminMode;
    void send_notification_message (const char* custom_status, char* played_game_name,char* gameserver_info);
};

void outer_function (void *arg);

#endif // MAINWINDOW_H
