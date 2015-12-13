#include "loginwindow.h"
#include "ui_loginwindow.h"

LoginWindow::LoginWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    m_Socket = new QTcpSocket();
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::on_RegisterButton_clicked()
{
    QString email, password;
    email = ui->EmailLineEdit->text();
    password = ui->PasswordLineEdit->text();

    if(email == "" || password == "") {
        ui->StatusLabel->setText("Please enter all the information!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    m_Socket->connectToHost("127.0.0.1", 1337);
    if(m_Socket->waitForConnected(1000))
        qDebug() << "Connected!";;
    if(m_Socket->state() == QAbstractSocket::UnconnectedState) {
        ui->StatusLabel->setText("Server is offline!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
    }

    QJsonObject user;
    user["connection"] = "0001";
    user["email"] = email;
    user["password"] = password;
    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));
    m_Socket->write(packet);
    m_Socket->flush();

    m_Socket->waitForReadyRead();
    packet = m_Socket->readAll();
    object = QJsonDocument::fromJson(packet.constData());
    user = object.object();

    if(user["connection"] == "0002") {
        ui->StatusLabel->setText("User with that Email adress already exists!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
    } else if (user["connection"] == "0003") {
        ui->StatusLabel->setText("User has been added!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : green; }");
    }

    m_Socket->disconnectFromHost();
    if(m_Socket->state() == QAbstractSocket::UnconnectedState
            || m_Socket->waitForDisconnected(1000))
        qDebug() << "Disconnected!";
    m_Socket->close();
}

void LoginWindow::on_LoginButton_clicked()
{
    QString email, password;
    email = ui->EmailLineEdit->text();
    password = ui->PasswordLineEdit->text();

    if(email == "" || password == "") {
        ui->StatusLabel->setText("Please enter all the information!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    if(!(m_Socket->state() == QAbstractSocket::ConnectedState)) {
        m_Socket->connectToHost("127.0.0.1", 1337);
        if(m_Socket->waitForConnected(1000))
            qDebug() << "Connected!";;
        if(m_Socket->state() == QAbstractSocket::UnconnectedState) {
            ui->StatusLabel->setText("Server is offline!");
            ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
        }
    }
    qint16 port = qrand() % 601 + 1400;
    QJsonObject user;
    user["connection"] = "0004";
    user["email"] = email;
    user["password"] = password;
    user["port"] = port;
    QJsonDocument object(user);
    QByteArray packet = (object.toJson(QJsonDocument::Compact));
    m_Socket->write(packet);
    m_Socket->flush();

    m_Socket->waitForReadyRead();
    packet = m_Socket->readAll();
    object = QJsonDocument::fromJson(packet.constData());
    user = object.object();

    if(user["connection"] == "0005") {
        ui->StatusLabel->setText("Sorry, that user does not seem to exist in our database!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
    } else if (user["connection"] == "0006") {
        ui->StatusLabel->setText("Wrong password!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
    }
    if(user["connection"] == "0007") {
        MainWindow* mainWin = new MainWindow(m_Socket, port);
        mainWin->show();
        this->close();
    }

}
