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
#include <QListWidgetItem>
#include <QMutex>
#include <QMap>
#include "ui_AddFriendWindow.h"
#include "ChatWindow.h"
#include "GameDetection.h"
#include "GameLibraryWindow.h"
#include "UsefulFunctions.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QTcpSocket* socket, bool adminMode, QString username, QWidget *parent = 0);
    ~MainWindow();
    void checkGameStatus();
    void refreshGamesList();
    void checkIfNewerGamesListExist();
    Ui::MainWindow *ui;
    QSound *snd;
    QMutex fileHandlingMutex;
    const QString username;
    QMap<QString, ChatWindow*> groupChatMap;
    GameLibraryWindow* gameLibWin = NULL;
    Ui::AddFriendWindow* addFriendBox = NULL;

public slots:

private slots:
    void on_addFriendButton_clicked();

    void on_actionConfigureGameLibrary_triggered();

    void on_currentStatusCBox_activated(const QString &arg1);

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_userStatsButton_clicked();

    void on_joinFriendButton_clicked();

    void on_actionMyStats_triggered();

    void on_instantChatButton_clicked();

    void on_actionDisconnect_triggered();

    void on_actionExit_triggered();

    void on_tableWidget_cellDoubleClicked(int row, int column);

    void onTcpMessageReceived();

private:
    QTcpSocket* socket;
    QString customStatus;  //defines custom status message which can user set and other can see (AFK, Selling piglets, Looking for match 2 vs 2)
    QString currentGame;   //defines game which user currently plays - empty string if none is played
    QMap<QString, ChatWindow*> privateChatMap;
    std::thread *gameActivityListenerThread;
    std::thread *globalShortcutListenerThread;
    bool adminMode;
    bool initialGamesListCheckingDone = false;
    void sendNotificationMessage (short tID, const char* customStatus, char* playedGameName, char* gameserverInfo);
    void showGameStats (QJsonObject object);
    void handleNewChatMessage (QJsonObject message);
    void refreshFriendsList (QJsonObject message);
    void requestGameActivityInfo (QString email);
    void requestFriendsList ();
    void getFriendsList(QJsonObject message);
    void updateSupportedGamesList(QJsonObject message, int packetSize);
    QString getFileLocationFromRegistryKey(QString registryKey);
    void autoDetectGames();
    void startProgram (const char* progName, const char* ip=NULL, const char* port=NULL);
    void processFriendRequest(QString username);
};

void listenGameActivity (void *arg);
void handleHotkeys (void *arg);

#endif // MAINWINDOW_H
