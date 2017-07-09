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
#include "funkcije.h"

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

    void refresh_friends_list();

    void CheckForMsg();

    void on_actionConfigure_game_library_triggered();

    void on_currentStatusCBox_activated(const QString &arg1);

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_UserStatsButton_clicked();

    void on_JoinFriendButton_clicked();

    void on_actionMy_Stats_triggered();

    void on_InstantChatButton_clicked();

    void on_actionDisconnect_triggered();

    void on_actionExit_triggered();

    void on_tableWidget_cellDoubleClicked(int row, int column);

private:
    QTcpSocket* m_Socket;
    QUdpSocket* m_UDPSocket;
    QString custom_status;  //defines custom status message which can user set and other can see (AFK, Selling piglets, Looking for match 2 vs 2)
    QString current_game;   //defines game which user currently plays - empty string if none is played
    QString m_Name;
    std::vector<ChatBox*> m_Chatvec;
    std::thread *gameActivityListenerThread;
    std::thread *globalShortcutListenerThread;
    qint16 m_Port;
    Ui::MainWindow *ui;
    bool adminMode;
    volatile bool right;        //variable has to be declared with 'volatile' qualifier because state (it's content) can be changed outside the thread which is checking it's value
    volatile bool flags[2];     //reason for using 'volatile' qualifier is same as previous one
    void send_notification_message (short tID, const char* custom_status, char* played_game_name, char* gameserver_info);
    void showGameStats (QString email);
    void enter_in_critical_section (bool tID1);
    void exit_from_critical_section (bool tID1);
};

void ListenGameActivity (void *arg);
void HandleHotkeys (void *arg);

#endif // MAINWINDOW_H
