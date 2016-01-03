#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QTcpServer>
#include "addfriend.h"
#include "chatbox.h"
#include "game_detection.h"
#include "launch_game.h"
#include <thread>
#include <QListWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QTcpSocket* socket, qint16 port, QWidget *parent = 0);
    ~MainWindow();
    static void update_state();
    static void send_notification_message (char* played_game_name,char* gameserver_info);


public slots:
    void NewConnection();

private slots:
    void on_AddFriendButton_clicked();

    void on_RefreshFriendsButton_clicked();

    void on_ChatButton_clicked();

    void on_actionConfigure_game_library_triggered();

    void on_currentStatusCBox_activated(const QString &arg1);

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_tabWidget_tabBarClicked(int index);

    void on_refreshButton_clicked();

private:
    QTcpSocket* m_Socket;
    QTcpServer* m_Server;
    qint16 m_Port;
    Ui::MainWindow *ui;
    std::thread *listener_thread;
    QString custom_status;  //defines custom status message which can user set and other can see (AFK, Selling piglets, Looking for match 2 vs 2)
    QString current_game;   //defines game which user currently plays - empty string if none is played
    void setOverallStatus();
};

void outer_function ();

#endif // MAINWINDOW_H
