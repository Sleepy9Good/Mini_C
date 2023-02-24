#ifndef TMWID_H
#define TMWID_H

#include <QWidget>
#include<tm.h>
#include<QString>
#include<QByteArray>
extern doCommand(void);

namespace Ui {
class tmwid;
}

class tmwid : public QWidget
{
    Q_OBJECT

public:
    explicit tmwid(QWidget *parent = 0);
    ~tmwid();

private:
    Ui::tmwid *ui;
};
void s_one_step();
void simshow(QString arr);
void input_my(char* inl);



#endif // TMWID_H
