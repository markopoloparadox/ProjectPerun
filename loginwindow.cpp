#include "loginwindow.h"
#include "ui_loginwindow.h"


LoginWindow::LoginWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    m_Socket = new QTcpSocket();

    connect(ui->RegisterButton, SIGNAL(clicked()), this, SLOT(RegisterAnAccount()));
    connect(ui->LoginButton, SIGNAL(clicked()), this, SLOT(Login()));
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::RegisterAnAccount() {
    /*
     * Qstring za razliku od std::string nam dopusta rad s unicodom, ima korisnije metode te
     * daje bolje performanse. Također, brojne funkcije koje kao ulaz traže znakovni niz, zapravo
     * traže QString tip podataka. Vrijednosti koji se nalaze u našim tekstualnim okvirima kopiramo
     * u pomoćne varijable koristeći metodu text();
     *
    */
    QString email, password;
    email = ui->EmailLineEdit->text();
    password = ui->PasswordLineEdit->text();

    //Treba upozoriti korisnika ako je jedan teksutalni okvir prazan
    if(email == "" || password == "") {
        ui->StatusLabel->setText("Please enter all the information!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
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
    m_Socket->connectToHost("127.0.0.1", 1337);
    if(m_Socket->waitForConnected(1000))
        qDebug() << "Connected!";
    else {
        qDebug() << "Not connected!";
        ui->StatusLabel->setText("Server is offline!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
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
    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(1000);
    m_Socket->waitForReadyRead(3000);

    /*
     * readALl() metoda čita bajtovni niz i sprema ga. Ostalo bi trebalo biti sve jasno
     *
    */
    packet = m_Socket->readAll();
    document = QJsonDocument::fromJson(packet.constData());
    object = document.object();
    if(object["connection"] == "0002") {
        ui->StatusLabel->setText("User with that Email adress already exists!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : blue; }");
    } else if (object["connection"] == "0003") {
        ui->StatusLabel->setText("User has been added!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : green; }");
    }

    /*
     * Odspajamo se i čekamo potvrdu da smo se otspojili. Metoda close() zatvara socket i resetira
     * ga.
     *
    */
    m_Socket->disconnectFromHost();
    if(m_Socket->state() == QAbstractSocket::UnconnectedState
        || m_Socket->waitForDisconnected(1000))
        qDebug() << "Disconnected!";
    m_Socket->close();

}

void LoginWindow::Login() {
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
        qDebug() << "Connected!";
    else {
        qDebug() << "Not connected!";
        ui->StatusLabel->setText("Server is offline!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
        return;
    }

    QJsonObject object;
    QJsonDocument document;
    QByteArray packet;
    quint16 port = qrand() % 601 + 1400;

    object["connection"] = "0004";
    object["email"] = email;
    object["password"] = password;
    object["port"] = port;
    document.setObject(object);
    packet = document.toJson(QJsonDocument::Compact);

    m_Socket->write(packet);
    m_Socket->waitForBytesWritten(1000);
    m_Socket->waitForReadyRead(3000);

    packet = m_Socket->readAll();
    document = QJsonDocument::fromJson(packet.constData());
    object = document.object();

    if(object["connection"] == "0005") {
        ui->StatusLabel->setText("Sorry, that user does not seem to exist in our database!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
    } else if (object["connection"] == "0006") {
        ui->StatusLabel->setText("Wrong password!");
        ui->StatusLabel->setStyleSheet("QLabel { background-color : white; color : red; }");
    } else if(object["connection"] == "0007") {
        MainWindow* mainWin = new MainWindow(m_Socket, port);
        mainWin->show();
        this->close();
    }

    m_Socket->disconnectFromHost();
    if(m_Socket->state() == QAbstractSocket::UnconnectedState
        || m_Socket->waitForDisconnected(1000))
        qDebug() << "Disconnected!";
    m_Socket->close();
}
