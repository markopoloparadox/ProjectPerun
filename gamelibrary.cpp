#include "gamelibrary.h"
#include "ui_gamelibrary.h"
#include "QFileDialog"
#include "QString"
#include "QMessageBox"
#include <string>
#include "funkcije.h"
#include "mainwindow.h"

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
    ((MainWindow*)this->parent())->fileHandlingMutex.lock();
    std::fstream file;
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    if (!file) {
        QMessageBox msgBox;
        msgBox.setText("File with names of supported games cannot be read. Check if the file \"gameslist.dat\" has been properly downloaded from the server.");
        msgBox.exec();
        file.close();
        ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
        return;
    }
    file.close();
    ((MainWindow*)this->parent())->fileHandlingMutex.unlock();

    this->numOfRows = 0;
    this->fill_table();
}

void gamelibrary::fill_table () {
    tGames gameRecord;
    std::fstream file;
    ((MainWindow*)this->parent())->fileHandlingMutex.lock();
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    while (this->numOfRows) {   //if there is any row remaining in table (which left before old table records were replaced with new ones), delete them
        ui->tableWidget->removeRow( --this->numOfRows );
    }
    ui->tableWidget->setSortingEnabled(false);  //sorting is disabled before initial insertion (due to performanse which can be gained with binary search used for inserting game paths) - reason is that columns in this table will be represented sorted by Full game name and records in file are stored by Process name
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //defines that it is not possible to select one cell only, but on mouse click (or on position through table items) is selected whole row
    ui->tableWidget->verticalHeader()->hide();  //hides row numbers

    while (true) {
        file.read( (char*)&gameRecord,sizeof(tGames) );
        if (file.eof()==true) {
            break;
        }

        ui->tableWidget->insertRow(this->numOfRows);    //this command inserts row, so cells can be added later in this loop

        ui->tableWidget->setItem(this->numOfRows,0,new QTableWidgetItem(gameRecord.fullName));
        ui->tableWidget->setItem(this->numOfRows,1,new QTableWidgetItem(gameRecord.processName));
        ui->tableWidget->setItem(this->numOfRows,2,new QTableWidgetItem(""));   //this muss to be here even if no path for some game is defined - reason is that that cell must contain object where values in future could be stored (new QTableWidgetItem)
        ui->tableWidget->setEditTriggers(0);    //this disables editing cells in table
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
        ui->tableWidget->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
        this->numOfRows++;
    }
    file.close();
    file.clear();
    std::fstream file2;
    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    file2.open("gameslist.dat",std::ios::in | std::ios::binary);
    tPath gamePath;
    while (true) {
        file.read( (char*)&gamePath,sizeof(tPath) );
        if (file.eof()==true) {
            break;
        }
        int result = binarySearchWrapper(file2,gamePath.processName);
        if (result != -1) {   //u svakom slučaju rezultat mora biti različit od -1 (jedino ako je datoteka gamepath.dat modificirana nekako drugacije od korisnikovog unosa iz ove aplikacije)
            ui->tableWidget->setItem(result,2,new QTableWidgetItem(gamePath.path));
        }
    }
    file.close();
    file.clear();
    file2.close();
    ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
    ui->tableWidget->setSortingEnabled(true);  //sorting is disabled before initial insertion (due to performanse which can be gained with binary search used for inserting game paths) - reason is that columns in this table will be represented sorted by Full game name and records in file are stored by Process name
    ui->tableWidget->sortByColumn(0,Qt::SortOrder::AscendingOrder);
}

gamelibrary::~gamelibrary()
{
    delete ui;
}

void gamelibrary::on_browseDestButton_clicked()
{
    int active_row=ui->tableWidget->currentRow();
    if (active_row==-1) {  //if none row is selected, tell user that (s)he must first select some row with specific game for which he wants to save changes
        QMessageBox msgBox;
        msgBox.setText("Select row in table which contains name of game to which you want to specify parameters!");
        msgBox.exec();
        return;
    }
    QString selected_process = ui->tableWidget->item(active_row,1)->text();

    QString path = QFileDialog::getOpenFileName(this, "Select file with following process name: " + selected_process,
                                   home_path,
                                   selected_process);
//    QString path = fd.getExistingDirectory(this, tr("Select directory that contains valid executable file"),
//                                                 home_path,
//                                                 QFileDialog::DontResolveSymlinks);
    path.resize(path.lastIndexOf('/') + 1);
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
          ((MainWindow*)this->parent())->fileHandlingMutex.lock();
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
                  int filesize = file.tellg();
                  file.seekp(filesize-sizeof(tPath),std::ios::beg);
                  break;
              }
          }
          strcpy(pathRecord.path , ui->destPathTextBox->text().toStdString().c_str());
          strcpy(pathRecord.customExecutableParameters , ui->custExecParam->text().toStdString().c_str());
          file.write( (char*)&pathRecord,sizeof(tPath) );
          file.close();
          ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
          ui->tableWidget->item(active_row,2)->setText( ui->destPathTextBox->text() );
    }
    else {
        ((MainWindow*)this->parent())->refresh_games_list();
        this->close();
    }
}

void gamelibrary::on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) //if cell is selected with mouse click, Tab positioning or arrow-keys positioning
{
    if (currentRow == -1) {     //this prevents program from being crashed when user clicks some button which immediately returns focus back on table's cell
        return;
    }
//    ui->tableWidget->selectRow(currentRow); //no need for this anymore because in class constructor is now defined that there is no individual cell selection, but whole row selection
    std::fstream file;
    tPath pathRecord;
    ((MainWindow*)this->parent())->fileHandlingMutex.lock();
    file.open("gamepath.dat",std::ios::in | std::ios::binary);
    while (true) {
        file.read( (char*)&pathRecord,sizeof(tPath) );
        if (file.eof()==true) {
            ui->destPathTextBox->clear();
            ui->custExecParam->clear();
            file.close();
            file.clear();
            ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
            return;
        }
        if (strcmp(ui->tableWidget->item(currentRow,1)->text().toUtf8(),pathRecord.processName)==0) {
            ui->destPathTextBox->setText(QString::fromUtf8(pathRecord.path));
            ui->custExecParam->setText(QString::fromUtf8(pathRecord.customExecutableParameters));
            break;
        }
    }
    file.close();
    ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
}

void gamelibrary::on_checkUpdateButton_clicked()
{
    ((MainWindow*)this->parent())->check_if_newer_games_list_exist();
}
