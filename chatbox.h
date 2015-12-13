#ifndef CHATBOX_H
#define CHATBOX_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace Ui {
class ChatBox;
}

class ChatBox : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatBox(QTcpSocket* socket = nullptr, QString ip = "", qint16 port = 0, QString myName = "", QString hisName = "", QWidget *parent = 0);
    ~ChatBox();

public slots:
    void Listen();

private slots:
    void on_EnterButton_clicked();

private:
    Ui::ChatBox *ui;
    QTcpSocket* m_Socket;
    QString m_IncClientName;
    QString m_OutClientName;
};

#endif // CHATBOX_H
