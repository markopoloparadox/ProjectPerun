#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include "mainwindow.h"

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = 0);
    ~LoginWindow();

private slots:
    void RegisterAnAccount();
    void Login();

    void on_PasswordLineEdit_returnPressed();

    void on_EmailLineEdit_returnPressed();

private:
    Ui::LoginWindow *ui;
    QTcpSocket* m_Socket;

};

#endif // LOGINWINDOW_H
