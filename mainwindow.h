#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUdpSocket>
#include <QSound>
#include <thread>
#include <vector>
#include <QListWidgetItem>
#include <QMutex>
#include <QMap>
#include "ui_addfriend.h"
#include "chatbox.h"
#include "game_detection.h"
#include "gamelibrary.h"
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
    void check_if_newer_games_list_exist();
    Ui::MainWindow *ui;
    QSound *snd;
    QMutex fileHandlingMutex;
    const QString m_Name;

public slots:

private slots:
    void on_AddFriendButton_clicked();

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

    void onTcpMessageReceived();

private:
    QTcpSocket* m_Socket;
    QString custom_status;  //defines custom status message which can user set and other can see (AFK, Selling piglets, Looking for match 2 vs 2)
    QString current_game;   //defines game which user currently plays - empty string if none is played
    std::vector<ChatBox*> chatGroupsVector;
    QMap<QString, ChatBox*> privateChatMap;
    std::thread *gameActivityListenerThread;
    std::thread *globalShortcutListenerThread;
    qint16 m_Port;
    bool adminMode;
    bool initialGamesListCheckingDone = false;
    gamelibrary* gameLibWin;
    Ui::AddFriend* addfriendbox;
    void send_notification_message (short tID, const char* custom_status, char* played_game_name, char* gameserver_info);
    void showGameStats (QJsonObject object);
    void process_new_chat_message (QJsonObject message);
    void refresh_friends_list (QJsonObject message);
    void requestGameActivityInfo (QString email);
    void request_friends_list ();
    void get_friends_list(QJsonObject message);
    void update_supported_games_list(QJsonObject message, int packetSize);
    QString getFileLocationFromRegistryKey(QString registryKey);
    void autoDetectGames();
    void start_program (const char* prog_name, const char* ip=NULL, const char* port=NULL);
};

void ListenGameActivity (void *arg);
void HandleHotkeys (void *arg);

#endif // MAINWINDOW_H
