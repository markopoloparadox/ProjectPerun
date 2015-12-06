#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "addfriend.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QTcpSocket* socket, QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_AddFriendButton_clicked();

    void on_RefreshFriendsButton_clicked();

private:
    QTcpSocket* m_Socket;
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
