#include "GameLibraryWindow.h"
#include "ui_GameLibraryWindow.h"
#include "QFileDialog"
#include "QString"
#include "QMessageBox"
#include <string>
#include "UsefulFunctions.h"
#include "MainWindow.h"

#if defined (__linux__)
QString homePath = "/home";
#endif

#if defined (_WIN32)
QString homePath = "C:/";
#endif

GameLibraryWindow::GameLibraryWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GameLibraryWindow),
    numOfRows(0)
{
    this->ui->setupUi(this);
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
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

    this->fillTable();
}

void GameLibraryWindow::fillTable () {
    tGames gameRecord;
    std::fstream file;
    ((MainWindow*)this->parent())->fileHandlingMutex.lock();
    file.open("gameslist.dat",std::ios::in | std::ios::binary);
    while (this->numOfRows) {   //if there is any row remaining in table (which left before old table records were replaced with new ones), delete them
        this->ui->tableWidget->removeRow( --this->numOfRows );
    }
    this->ui->tableWidget->setSortingEnabled(false);  //sorting is disabled before initial insertion (due to performanse which can be gained with binary search used for inserting game paths) - reason is that columns in this table will be represented sorted by Full game name and records in file are stored by Process name
    this->ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);   //defines that it is not possible to select one cell only, but on mouse click (or on position through table items) is selected whole row
    this->ui->tableWidget->verticalHeader()->hide();  //hides row numbers

    while (true) {
        file.read( (char*)&gameRecord,sizeof(tGames) );
        if (file.eof()) {
            break;
        }

        this->ui->tableWidget->insertRow(this->numOfRows);    //this command inserts row, so cells can be added later in this loop

        this->ui->tableWidget->setItem(this->numOfRows,0,new QTableWidgetItem(gameRecord.fullName));
        this->ui->tableWidget->setItem(this->numOfRows,1,new QTableWidgetItem(gameRecord.processName));
        this->ui->tableWidget->setItem(this->numOfRows,2,new QTableWidgetItem(""));   //this must to be here even if no path for some game is defined - reason is that that cell must contain object where values in future could be stored (new QTableWidgetItem)
        this->ui->tableWidget->setEditTriggers(0);    //this disables editing cells in table
        this->ui->tableWidget->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
        this->ui->tableWidget->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
        this->ui->tableWidget->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);
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
        if (file.eof()) {
            break;
        }
        int result = binarySearchWrapper(file2,gamePath.processName);
        if (result != -1) {   //u svakom slučaju rezultat mora biti različit od -1 (jedino ako je datoteka gamepath.dat modificirana nekako drugacije od korisnikovog unosa iz ove aplikacije)
            this->ui->tableWidget->setItem(result,2,new QTableWidgetItem(gamePath.path));
        }
    }
    file.close();
    file.clear();
    file2.close();
    ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
    this->ui->tableWidget->setSortingEnabled(true);  //sorting is disabled before initial insertion (due to performanse which can be gained with binary search used for inserting game paths) - reason is that columns in this table will be represented sorted by Full game name and records in file are stored by Process name
    this->ui->tableWidget->sortByColumn(0,Qt::SortOrder::AscendingOrder);
}

GameLibraryWindow::~GameLibraryWindow()
{
    ((MainWindow*)this->parent())->gameLibWin = NULL;
    delete ui;
}

void GameLibraryWindow::on_browseDestButton_clicked()
{
    int activeRow = this->ui->tableWidget->currentRow();
    if (activeRow==-1) {  //if none row is selected, tell user that (s)he must first select some row with specific game for which he wants to save changes
        QMessageBox msgBox;
        msgBox.setText("Select row in table which contains name of game to which you want to specify parameters!");
        msgBox.exec();
        return;
    }
    QString selectedProcess = this->ui->tableWidget->item(activeRow,1)->text();

    QString path = QFileDialog::getOpenFileName(this, "Select file with following process name: " + selectedProcess,
                                   homePath,
                                   selectedProcess);
//    QString path = fd.getExistingDirectory(this, tr("Select directory that contains valid executable file"),
//                                                 home_path,
//                                                 QFileDialog::DontResolveSymlinks);
    path.resize(path.lastIndexOf('/') + 1);
    ui->destPathTextBox->setText(path);
}

void GameLibraryWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    if((QPushButton *)button == this->ui->buttonBox->button(QDialogButtonBox::Apply) ) {
          int activeRow = this->ui->tableWidget->currentRow();
          if (activeRow==-1) {  //if none row is selected, tell user that (s)he must first select some row with specific game for which he wants to save changes
              QMessageBox msgBox;
              msgBox.setText("Select row in table which contains name of game to which you want to specify parameters!");
              msgBox.exec();
              return;
          }
          std::string selected_process = this->ui->tableWidget->item(activeRow,1)->text().toStdString();
          tPath pathRecord;
          std::fstream file;
          ((MainWindow*)this->parent())->fileHandlingMutex.lock();
          file.open("gamepath.dat",std::ios::in | std::ios::out | std::ios::binary);
          while (true) {
              file.read( (char*)&pathRecord,sizeof(tPath) );
              if (file.eof()) {
                  file.clear();
                  file.seekp(0,std::ios::end);
                  strcpy(pathRecord.processName , selected_process.c_str());
                  break;
              }
              if (strcmp(selected_process.c_str(),pathRecord.processName)==0) {
                  int fileSize = file.tellg();
                  file.seekp(fileSize-sizeof(tPath),std::ios::beg);
                  break;
              }
          }
          strcpy(pathRecord.path , this->ui->destPathTextBox->text().toStdString().c_str());
          strcpy(pathRecord.customExecutableParameters , this->ui->custExecParam->text().toStdString().c_str());
          file.write( (char*)&pathRecord,sizeof(tPath) );
          file.close();
          ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
          this->ui->tableWidget->item(activeRow,2)->setText( this->ui->destPathTextBox->text() );
    }
    else {
        ((MainWindow*)this->parent())->refreshGamesList();
        this->close();
    }
}

void GameLibraryWindow::on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) //if cell is selected with mouse click, Tab positioning or arrow-keys positioning
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
        if (file.eof()) {
            this->ui->destPathTextBox->clear();
            this->ui->custExecParam->clear();
            file.close();
            file.clear();
            ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
            return;
        }
        if (strcmp(this->ui->tableWidget->item(currentRow,1)->text().toUtf8(),pathRecord.processName)==0) {
            this->ui->destPathTextBox->setText(QString::fromUtf8(pathRecord.path));
            this->ui->custExecParam->setText(QString::fromUtf8(pathRecord.customExecutableParameters));
            break;
        }
    }
    file.close();
    ((MainWindow*)this->parent())->fileHandlingMutex.unlock();
}

void GameLibraryWindow::on_checkUpdateButton_clicked()
{
    ((MainWindow*)this->parent())->checkIfNewerGamesListExist();
}
