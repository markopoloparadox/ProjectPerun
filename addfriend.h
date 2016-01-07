#ifndef ADDFRIEND_H
#define ADDFRIEND_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace Ui {
class AddFriend;
}

class AddFriend : public QMainWindow
{
    Q_OBJECT

public:
    explicit AddFriend(QTcpSocket* socket, QWidget *parent = 0);
    ~AddFriend();

private slots:
    void on_AddButton_clicked();

private:
    Ui::AddFriend *ui;
    QTcpSocket* m_Socket;
};

#endif // ADDFRIEND_H
