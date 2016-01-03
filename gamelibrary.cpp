#include "gamelibrary.h"
#include "ui_gamelibrary.h"
#include "QFileDialog"
#include "QString"
#include "QMessageBox"
#include <string>
#include "funkcije.h"


#if defined (__linux__)
QString home_path="/home";
#endif

#if defined (_WIN32)
QString home_path="C:/";
#endif

gamelibrary::gamelibrary(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::gamelibrary)
{
    ui->setupUi(this);
    tGames gameRecord;
    std::fstream file;
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    if (!file) {
        QMessageBox msgBox;
        msgBox.setText("File with names of supported games cannot be read. Check if the file \"gameslist.dat\" has been properly downloaded from the server.");
        msgBox.exec();
        return;
    }
    int counter=0;
//    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //defines that it is not possible to select one cell only, but on mouse click (or on position through table items) is selected whole row
//    ui->tableWidget->verticalHeader()->hide();  //hides row numbers
    while (true) {
        file.read( (char*)&gameRecord,sizeof(tGames) );
        if (file.eof()==true) {
            break;
        }

        ui->tableWidget->insertRow(counter);    //this command inserts rows, so cells can be added later in this loop

        ui->tableWidget->setItem(counter,0,new QTableWidgetItem(gameRecord.fullName));
        ui->tableWidget->setItem(counter,1,new QTableWidgetItem(gameRecord.processName));
        ui->tableWidget->setItem(counter,2,new QTableWidgetItem(""));   //this muss to be here even if no path for some game is defined - reason is that that cell must contain object where values in future could be stored (new QTableWidgetItem)
        ui->tableWidget->setEditTriggers(0);    //this disables editing cells in table
        counter++;
    }
    file.close();
    file.clear();
    std::fstream file2;
    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    file2.open("gameslist.dat",std::ios::in | std::ios::binary);
    tPath gamePath;
    counter=0;
    while (true) {
        file.read( (char*)&gamePath,sizeof(tPath) );
        if (file.eof()==true) {
            break;
        }
        int result=binarySearchWrapper(file2,gamePath.processName);
        if (result!=-1) {   //u svakom slučaju rezultat mora biti različit od -1 (jedino ako je datoteka gamepath.dat modificirana nekako drugacije od korisnikovog unosa iz ove aplikacije)
            ui->tableWidget->setItem(result,2,new QTableWidgetItem(gamePath.path));
        }
    }
    file.close();
    file2.close();
}

gamelibrary::~gamelibrary()
{
    delete ui;
}

void gamelibrary::on_browseDestButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 home_path,
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    ui->destPathTextBox->setText(path);
}

void gamelibrary::on_buttonBox_clicked(QAbstractButton *button)
{
    if((QPushButton *)button== ui->buttonBox->button(QDialogButtonBox::Apply) ) {
          int active_row=ui->tableWidget->currentRow();
          if (active_row==-1) {  //if none row is selected, tell user that (s)he must first select some row with specific game for which he wants to save changes
              QMessageBox msgBox;
              msgBox.setText("Select row in table which contains name of game to which you want to specify parameters!");
              msgBox.exec();
              return;
          }
          std::string selected_process=ui->tableWidget->item(active_row,1)->text().toStdString();
          tPath pathRecord;
          std::fstream file;
          file.open("gamepath.dat",std::ios::in | std::ios::out | std::ios::binary);
          while (true) {
              file.read( (char*)&pathRecord,sizeof(tPath) );
              if (file.eof()==true) {
                  file.clear();
                  file.seekp(0,std::ios::end);
                  strcpy(pathRecord.processName , selected_process.c_str());
                  break;
              }
              if (strcmp(selected_process.c_str(),pathRecord.processName)==0) {
                  file.seekp(file.tellg()-sizeof(tPath),std::ios::beg);
                  break;
              }
          }
          strcpy(pathRecord.path , ui->destPathTextBox->text().toStdString().c_str());
          strcpy(pathRecord.customExecutableParameters , ui->custExecParam->text().toStdString().c_str());
          file.write( (char*)&pathRecord,sizeof(tPath) );
          file.close();
          ui->tableWidget->item(active_row,2)->setText( ui->destPathTextBox->text() );
    }
    else {
        this->close();
    }
}

void gamelibrary::on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) //if cell is selected with mouse click, Tab positioning or arrow-keys positioning
{
//    ui->tableWidget->selectRow(currentRow); //no need for this anymore because in class constructor is now defined that there is no individual cell selection, but whole row selection
    std::fstream file;
    tPath pathRecord;
    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    while (true) {
        file.read( (char*)&pathRecord,sizeof(tPath) );
        if (file.eof()==true) {
            ui->destPathTextBox->clear();
            ui->custExecParam->clear();
            file.close();
            file.clear();
            return;
        }
        if (strcmp(ui->tableWidget->item(currentRow,1)->text().toUtf8(),pathRecord.processName)==0) {
            ui->destPathTextBox->setText(QString::fromUtf8(pathRecord.path));
            ui->custExecParam->setText(QString::fromUtf8(pathRecord.customExecutableParameters));
            break;
        }
    }
    file.close();
}
