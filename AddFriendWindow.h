#ifndef ADDFRIEND_H
#define ADDFRIEND_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace Ui {
class AddFriendWindow;
}

class AddFriendWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AddFriendWindow(QTcpSocket* socket, QWidget *parent = 0);
    ~AddFriendWindow();
    Ui::AddFriendWindow* ui;

private slots:
    void on_addButton_clicked();

private:
    QTcpSocket* socket;
};

#endif // ADDFRIEND_H
