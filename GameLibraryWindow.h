#ifndef GAMELIBRARY_H
#define GAMELIBRARY_H

#include <QDialog>
#include <QAbstractButton>
#include "UsefulFunctions.h"

namespace Ui {
class GameLibraryWindow;
}

class GameLibraryWindow : public QDialog
{
    Q_OBJECT

public:
    explicit GameLibraryWindow(QMap<QString, tPath>& detectedGames, QWidget *parent = 0);
    ~GameLibraryWindow();
    void fillTable();

private slots:

    void on_browseDestButton_clicked();

    void on_buttonBox_clicked(QAbstractButton *button);
    
    void on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_checkUpdateButton_clicked();

private:
    Ui::GameLibraryWindow *ui;
    short numOfRows;
    QMap<QString, tPath>& detectedGames;
};

#endif // GAMELIBRARY_H
