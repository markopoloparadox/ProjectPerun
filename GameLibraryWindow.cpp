#include "GameLibraryWindow.h"
#include "ui_GameLibraryWindow.h"
#include "QFileDialog"
#include "QString"
#include "QMessageBox"
#include <string>
#include "MainWindow.h"

#if defined (__linux__)
QString homePath = "/home";
#endif

#if defined (_WIN32)
QString homePath = "C:/";
#endif

GameLibraryWindow::GameLibraryWindow(QMap<QString, tPath>& detectedGames, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GameLibraryWindow),
    numOfRows(0),
    detectedGames(detectedGames)
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
        if (detectedGames.contains(gameRecord.processName)) {
            this->ui->tableWidget->setItem(this->numOfRows,2,new QTableWidgetItem(detectedGames[gameRecord.processName].path));
        }
        else {
            this->ui->tableWidget->setItem(this->numOfRows,2,new QTableWidgetItem(""));
        }
        this->ui->tableWidget->setEditTriggers(0);    //this disables editing cells in table
        this->ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        this->ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
        this->numOfRows++;
    }
    file.clear();
    file.close();
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
          QString selectedProcess = this->ui->tableWidget->item(activeRow,1)->text();
          if (this->ui->destPathTextBox->text().trimmed().isEmpty()) {
              detectedGames.remove(selectedProcess);
          }
          else {
              tPath pathRecord;
              strcpy(pathRecord.processName, selectedProcess.toStdString().c_str());
              strcpy(pathRecord.path , this->ui->destPathTextBox->text().toStdString().c_str());
              strcpy(pathRecord.customExecutableParameters , this->ui->custExecParam->text().toStdString().c_str());
              detectedGames[selectedProcess] = pathRecord;
          }
          std::fstream file;
          ((MainWindow*)this->parent())->fileHandlingMutex.lock();
          file.open("gamepath.dat", std::ios::out | std::ios::binary);
          for (tPath pathRecord : detectedGames) {
              file.write( (char*)&pathRecord,sizeof(tPath) );
          }
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
    tPath pathRecord;
    QString selectedProcess = this->ui->tableWidget->item(currentRow,1)->text();
    if (detectedGames.contains(selectedProcess)) {
        tPath pathRecord = detectedGames[selectedProcess];
        this->ui->destPathTextBox->setText(QString::fromUtf8(pathRecord.path));
        this->ui->custExecParam->setText(QString::fromUtf8(pathRecord.customExecutableParameters));
    }
    else {
        this->ui->destPathTextBox->clear();
        this->ui->custExecParam->clear();
    }
}

void GameLibraryWindow::on_checkUpdateButton_clicked()
{
    ((MainWindow*)this->parent())->checkIfNewerGamesListExist();
}
