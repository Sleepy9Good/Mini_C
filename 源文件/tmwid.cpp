#include "tmwid.h"
#include "ui_tmwid.h"
#include "tm.h"
#include"string.h"
#include <QCoreApplication>
#include <QWaitCondition>
#include <QMutex>
#include <QThread>
#include<QMessageBox>
#include<QMutex>
Ui::tmwid *curui=NULL;
tmwid::tmwid(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::tmwid)
{
    curui = ui;
    ui->setupUi(this);
    ui->inputbutt->setFocus();
//    ui->inputbutt->setShortcut( QKeySequence::InsertParagraphSeparator );  //设置快捷键为键盘的“回车”键
    ui->inputbutt->setShortcut(Qt::Key_Enter);  //设置快捷键为enter键
    ui->inputbutt->setShortcut(Qt::Key_Return); //设置快捷键为小键盘上的enter键
    connect(ui->inputbutt,&QPushButton::clicked,[=](){//输入
        if(ui->inputline->text()=="") {QMessageBox::about(NULL,"反馈","请先输入输入项");return;}
        s_one_step();
    });
    connect(ui->clearbutt,&QPushButton::clicked,[=](){//清屏
        ui->simtext->clear();
    });

    printf("TM  simulation (enter g for run, enter h for help)...\n");
    simshow("TM  simulation (enter g for run, enter h for help)...\n");

}

tmwid::~tmwid()
{
    delete ui;
}
void simshow(QString arr){
    QByteArray str1 = arr.toLatin1();
    curui->simtext->textCursor().insertText(str1);
}

void input_my(char* inl){
    QString arr = curui->inputline->text();
    char* chrs = arr.toLatin1().data();
    strcpy(inl,chrs);
    simshow("\ninput: ");
    simshow(arr);simshow("\n");
    curui->inputline->clear();
}

void s_one_step(){
    if(!stefla){
        printf("Enter command: ");
        simshow("Enter command: ");
        fflush(stdin);
        fflush(stdout);
        if(!done)
          done = !doCommand();
        else if(!done){
            printf("Simulation done.\n");
            simshow("Simulation done.\n");
            stefla=true;
        }
    }
    else   fclose(pgm);
}


