#include "LoginWindow.h"
#include "ui_LoginWindow.h"
#include <QMessageBox>


LoginWindow::LoginWindow(bool adminMode, QWidget *parent) :
    QMainWindow(parent),
    socket(new QTcpSocket()),
    adminMode(adminMode),    //value of adminMode as a parameter (that was sent from main() function) is set to adminMode attribute of LoginWindow (that's required because it will be forwarded next to instance of MainWindow class)
    ui(new Ui::LoginWindow)
{
    this->ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog);
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    this->ui->passwordLineEdit->setEchoMode(QLineEdit::Password); //hides entered characters while typing

    connect(this->ui->registerButton, SIGNAL(clicked()), this, SLOT(registerAnAccount()));
    connect(this->ui->loginButton, SIGNAL(clicked()), this, SLOT(login()));
    connect(this->ui->passwordLineEdit, SIGNAL(returnPressed()), this, SLOT(login()));
    connect(this->ui->emailLineEdit, SIGNAL(returnPressed()), this, SLOT(login()));

    if (!adminMode) {
        QMessageBox msgBox;
        msgBox.setText("To use all features that application contains, please run this application with administative privileges.");
        msgBox.exec();
    }
}

LoginWindow::~LoginWindow()
{
    delete this->ui;
}

void LoginWindow::registerAnAccount() {
    /*
     * Qstring za razliku od std::string nam dopusta rad s unicodom, ima korisnije metode te
     * daje bolje performanse. Također, brojne funkcije koje kao ulaz traže znakovni niz, zapravo
     * traže QString tip podataka. Vrijednosti koji se nalaze u našim tekstualnim okvirima kopiramo
     * u pomoćne varijable koristeći metodu text();
     *
    */
    QString email, password;
    email = this->ui->emailLineEdit->text();
    password = this->ui->passwordLineEdit->text();

    //Treba upozoriti korisnika ako je jedan teksutalni okvir prazan
    if(email.isEmpty() || password.isEmpty()) {
        this->ui->statusLabel->setText("Please enter all the information!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    /*
     * Metoda connectToHost(Qstring, quint16,...) kao prvi argument prima ip adresu koja može
     * biti u dva oblika ("192.168.0.1") ili ("mywebpage.com") a kao drugi broj porta.
     * Sljedeći parametri su neobavezni pa ih neću niti napomenuti.
     * Kako bi se program izvodio sinkronizirano (jer se u suprotnom može dogoditi da šaljemo
     * podatke a još se nismo spojiti) koristimo metodu waitForConnected(ms) koja nakon što je
     * primljen signal da smo spojeni vraća true odnosno vraća false nakon što istekne određeno
     * vrijeme(timeout je određen brojem u zagradi).
     *
    */
    this->socket->connectToHost(this->serverAddress, this->serverPort);
    if(this->socket->waitForConnected(3000))
        qDebug() << "Connection with server is successfully established!";
    else {
        qDebug() << "Connection with server could not be established!";
        this->ui->statusLabel->setText("Server is offline!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
        return;
    }

    /*
     * QJsonObject je objekt koji se sastoji od više QJsonValue. QJsonObject možemo gledati kao
     * klasu koje se može sastojati od različitih tipova podataka, te mi možemo proizvoljno
     * dodavati elemente koje želimo. Za dodavanje elementa u QJsonObject korisimo ovu sintaxu
     * QJsonObject["key"] = example.
     *
     * QJsonDocument je srce i pokazatelj moći JSON strukture. QJsonDocument kao argumenat prima
     * QJsonObject ili QJsonArray(isti efekt možemo dobiti ako koristimo metode setObject() ili
     * setArray()) te taj objekt(ondosno array) postavlja kao primarni dokument. Koristeći metodu
     * toJson(format), vraća svoj dokumenat u obliku QByteArray-a a metodom fromJson(QByteArray)
     * pretvara QByteArray u QJsonDocument.
     *
     * QByteArray je laički gledano isto kao i QString. Koristimo ga ovjde isključivo zato što
     * funkcije traže takav tip podatakaali moglii smo također i QString koristiti.
     *
     * Postoji dva tipa prikaza JSON datoteka. Compact i Indented. Dovoljno je znati da Compact
     * je kompaktan i "ljepši" je prikaz za računala. Za ljude je bolji Indented prikaz.
     * http://doc.qt.io/qt-5/qjsondocument.html#JsonFormat-enum
     *
    */
    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0001";
    object["email"] = email;
    object["password"] = password;
    document.setObject(object);
    packet = document.toJson(QJsonDocument::Compact);

    /*
     * Write(QByteArray) metoda šalje sadržaj dok waitForBytesWritten(timeout) čeka dok svi
     * podaci ne budu poslani (ili do poajve signala). WaitForReadyRead(timeout) čeka dok se ne
     * pojavi signal readyRead() signal. Obje wait metode također blokiraju rad programa
     * odnosno vrši se sinkronizacija.
     *
    */
    this->socket->write(packet);
    this->socket->waitForBytesWritten(1000);
    this->socket->waitForReadyRead(3000);

    /*
     * readAll() metoda čita bajtovni niz i sprema ga. Ostalo bi trebalo biti sve jasno
     *
    */
    packet = this->socket->readAll();
    document = QJsonDocument::fromJson(packet.constData());
    object = document.object();
    if(object["connection"] == "0002") {
        this->ui->statusLabel->setText("User with that Email adress already exists!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
    } else if (object["connection"] == "0003") {
        this->ui->statusLabel->setText("User has been added!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : green; }");
    }

    /*
     * Odspajamo se i čekamo potvrdu da smo se odspojili. Metoda close() zatvara socket i resetira
     * ga.
     *
    */
    this->socket->disconnectFromHost();
    if(this->socket->state() == QAbstractSocket::UnconnectedState || this->socket->waitForDisconnected(1000)) {
        qDebug() << "Disconnected!";
    }
    this->socket->close();

}

void LoginWindow::login() {
    QString email, password;
    email = this->ui->emailLineEdit->text();
    password = this->ui->passwordLineEdit->text();

    if(email.isEmpty() || password.isEmpty()) {
        this->ui->statusLabel->setText("Please enter all the information!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
        return;
    }

    this->socket->connectToHost(this->serverAddress, this->serverPort);
    if(this->socket->waitForConnected(3000))
        qDebug() << "Connection with server is successfully established!";
    else {
        qDebug() << "Connection with server could not be established!";
        this->ui->statusLabel->setText("Server is offline!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
        return;
    }

    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;

    object["connection"] = "0004";
    object["email"] = email;
    object["password"] = password;
    document.setObject(object);
    packet = document.toJson(QJsonDocument::Compact);

    this->socket->write(packet);
    this->socket->waitForBytesWritten(1000);
    this->socket->waitForReadyRead(3000);

    packet = this->socket->readAll();
    document = QJsonDocument::fromJson(packet.constData());
    object = document.object();

    if(object["connection"] == "0005") {
        this->ui->statusLabel->setText("Sorry, that user does not seem to exist in our database!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
    } else if (object["connection"] == "0006") {
        this->ui->statusLabel->setText("Wrong password!");
        this->ui->statusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
    } else if(object["connection"] == "0007") {
        MainWindow* mainWin = new MainWindow(this->socket, this->adminMode, email);
        //mainWin->setAttribute(Qt::WA_DeleteOnClose);
        mainWin->show();
        this->close();
    }
}
