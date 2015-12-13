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

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QTcpSocket* socket, qint16 port, QWidget *parent = 0);
    ~MainWindow();

public slots:
    void NewConnection();

private slots:

    void on_AddFriendButton_clicked();

    void on_RefreshFriendsButton_clicked();

    void on_ChatButton_clicked();


private:
    QTcpSocket* m_Socket;
    QTcpServer* m_Server;
    qint16 m_Port;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
