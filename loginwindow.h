#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTime>
#include "mainwindow.h"

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoginWindow(bool adminMode, QWidget *parent = 0);
    ~LoginWindow();

private slots:
    void RegisterAnAccount();

    void Login();

private:
    Ui::LoginWindow *ui;
    QTcpSocket* m_Socket;
    bool adminMode;

};

#endif // LOGINWINDOW_H
