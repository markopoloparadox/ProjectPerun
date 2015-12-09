#ifndef CHATBOX_H
#define CHATBOX_H

#include <QMainWindow>

namespace Ui {
class ChatBox;
}

class ChatBox : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatBox(QWidget *parent = 0);
    ~ChatBox();

private:
    Ui::ChatBox *ui;
};

#endif // CHATBOX_H
