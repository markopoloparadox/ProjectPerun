#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTime>
#include "MainWindow.h"

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoginWindow(bool adminMode, const char* const serverAddress, const short serverPort, QWidget *parent = 0);
    ~LoginWindow();

private slots:
    void registerAnAccount();

    void login();

private:
    Ui::LoginWindow *ui;
    QTcpSocket* socket;
    bool adminMode;
    const char* const serverAddress;
    const ushort serverPort;
};

#endif // LOGINWINDOW_H
