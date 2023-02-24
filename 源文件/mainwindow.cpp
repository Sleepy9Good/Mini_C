#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tmwid.h"
#include"global.h"
#include"parse.h"
#include"scan.h"
#include "analyse.h"
#include "cgen.h"
#include "tm.h"
#include"code.h"
#include<time.h>
#include<iostream>

#include<QMessageBox>
#include<QString>
#include<QByteArray>
#include<QFileDialog>
#include<QFile>
#include<QStandardItem>
#include<QTextStream>
#include<fstream>
bool fileflag = false;//标记文件打开状态
bool parseflag = false;//标记分析状态
QFile file;
QStringList fileNames;
QByteArray tempba;


Ui::MainWindow *curUi=NULL;
void codPnt(QString ar){
    QByteArray arr = ar.toLatin1();
    curUi->codetext_2->textCursor().insertText(arr);
}
void trePnt(QString ar){
    QByteArray arr = ar.toLatin1();
    curUi->treetext->textCursor().insertText(arr);
}



using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    curUi=ui;
    ui->setupUi(this);
    ui->filename->setText("当前为空，请读取");

    connect(ui->exit,&QPushButton::clicked,this,&MainWindow::close);//退出
    connect(ui->readbutton,&QPushButton::clicked,[=](){//读取文件
        QFileDialog *fileDialog = new QFileDialog(this);
        fileflag = true;
        fileNames.clear();
        //选择文件读取
        fileDialog->setWindowTitle(QStringLiteral("选择一个文件"));//定义文件对话框标题
        fileDialog->setViewMode(QFileDialog::Detail); //设置视图模式
        if (fileDialog->exec()) {fileNames = fileDialog->selectedFiles();}
        if (fileNames.isEmpty()){
            return;
        }
        QString filepath = fileNames[0];
        ui->filename->setText(filepath);
        //读取
        file.setFileName(filepath);
        file.open(QIODevice::ReadOnly|QIODevice::Text);//以只读的方式打开文本文件
        if(!file.isOpen())
        {
            QMessageBox::about(NULL,"提示","open file failed!!!");
            return;
        }
        QByteArray ba2;
        ba2 = filepath.toLatin1();
        const char *c2 = ba2.data();
        ui->codetext->clear();
        int k = 1;
        while (!file.atEnd()) {
            QByteArray arr = file.readLine();
            QString str = QString::number(k) + " " + QString(arr);
            arr = str.toLatin1();
            k++;
            ui->codetext->textCursor().insertText(arr);
        }
        file.close();

    });
    connect(ui->clearbutton,&QPushButton::clicked,[=](){//清空文件
        file.close();
        fileflag = false;
        parseflag = false;
        fileNames.clear();
        ui->filename->setText("当前为空，请读取");
        ui->codetext->clear();
        ui->codetext_2->clear();
//        ui->parsetext->clear();
        for(int i  = 0; i < 211; i++){
            nationTable[i] = NULL;
        }
        for(int i  = 0; i < 261; i++){
            funcEnList[i] = NULL;
        }
        for(int i  = 0; i < 212; i++){
            offsetArr[i] = NULL;
        }
        for(int i  = 0; i < 14; i++){
            tabList[i] = NULL;
        }

        //全局变量重置
        traceflag = FALSE;
        icountflag = FALSE;
        isFloat = FALSE;
        {
            iloc = 0;
            dloc = 0;
            for (int regNo = 0; regNo < 8; regNo++) {
                reg[regNo].isFlo = FALSE;
                reg[regNo].val._int = 0;
            }
            dMem[0].isFlo = FALSE;
            dMem[0].val._int = 1024 - 1;
            for (int loc = 1; loc < 1024; loc++) {
                dMem[loc].isFlo = FALSE;
                dMem[loc].val._int = 0;
            }
        }
        curFuncName = NULL;
        returnType = Void;
        level = 0;
        tmpOffset = 0;
        emitLoc = 0;
        highEmitLoc = 0;
        offset = 0;
        lineno = 0;
        linepos = 0;
        bufsize = 0;
        EOF_flag = false;

        ui->treetext->clear();
        lineno = 0;
    });
    connect(ui->parsebutton,&QPushButton::clicked,[=](){//源码分析,生成语法树和中间代码
        if(fileflag==false){
            QMessageBox::about(NULL,"提示","请先读取文件");
            return;
        }
        parseflag = true;
        QString filepath = fileNames[0];
        tempba = filepath.toLatin1();
        char* ch_file = tempba.data();
        ui->codetext_2->clear();
        ui->treetext->clear();
        //语法分析、生成显示语法树
        FILE* code;
        fopen_s(&code, ch_file, "rb");
        if (code == NULL) {
            printf("源文件打开失败，请检查！");
            QMessageBox::about(NULL,"提示",QString(ch_file)+"打开失败，请检查！");
            exit(1);
        }
        FILE* tmpf;
        fopen_s(&source, "D:\\tmp1.txt", "w+");
        fopen_s(&tmpf, "D:\\tmp2.txt", "w+");
//        source = tmpfile();
//        FILE* tmpf = tmpfile();
        if (tmpf == NULL || source == NULL) {
            printf("临时文件创建失败！");
            QMessageBox::about(NULL,"提示","临时文件创建失败");
            return;
        }
        /* 将input函和output的函数定义插入文件头部*/
        char str[] =
        "int input(void)\n\{\n\
        int n;\n\
        INPUT n;\n\
        return n;\n}\n\nvoid output(int x)\n{\n\
        OUTPUT x;\n}\n\n";
        fputs(str, tmpf);
        char tmp[256] = { '\0' };
        while (fgets(tmp, 255, code)) {
            fputs(tmp, tmpf);
        }
        fclose(code);
        rewind(tmpf);
        while ((fgets(tmp, 255, tmpf))) {
            fputs(tmp, source);
        }
        rewind(source);
        /* 遍历源文件生成语法树*/
        TreeNode* t = parse();
        /* 根据语法树生成各级符号表，并进行类型检查*/
        buildSymtab(t, nationTable);
        if ((output = fopen("D:\\output_tmpfile_use.ntm", "w")) == NULL) {
                printf("输出文件创建失败，请检查硬盘空间是否充足！\n");
                QMessageBox::about(NULL,"提示","输出文件创建失败，请检查硬盘空间是否充足！");
                exit(1);
            }
            /* 根据语法树生成中间代码*/
        codeGen(t, ch_file);
        fclose(output);
        fclose(source);
        fclose(tmpf);
    });
    connect(ui->treSavBtn,&QPushButton::clicked,[=](){//保存语法树
        if(parseflag==false){
            QMessageBox::about(NULL,"提示","请先分析源码");
            return;
        }
        QString fileName = QFileDialog::getSaveFileName(this,
                tr("保存语法树"),
                "",
                tr("text Files (*.txt)"));
            if (!fileName.isNull())
            {
                QFile outfile(fileName);
                outfile.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate);
                QTextStream out(&outfile);
                out<<ui->treetext->toPlainText()<<'\n';
                outfile.close();
                QMessageBox::about(NULL, "提示", "语法树保存完成");
            }
            else
            {
                QMessageBox::about(NULL, "提示", "请输入保存文件名字");
            }
    });
    connect(ui->codesave,&QPushButton::clicked,[=](){//保存中间代码
        if(parseflag==false){
            QMessageBox::about(NULL,"提示","请先分析源码");
            return;
        }
        QString fileName = QFileDialog::getSaveFileName(this,
                tr("保存中间代码"),
                "",
                tr("text Files (*.txt)"));
            if (!fileName.isNull())
            {
                QFile outfile(fileName);
                outfile.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate);
                QTextStream out(&outfile);
                out<<ui->codetext_2->toPlainText()<<'\n';
                outfile.close();
                QMessageBox::about(NULL, "提示", "中间代码保存完成");
            }
            else
            {
                QMessageBox::about(NULL, "提示", "请输入保存文件名字");
            }
    });
    connect(ui->code_run,&QPushButton::clicked,[=](){//执行中间代码
        if(parseflag==false){
            QMessageBox::about(NULL,"提示","请先分析源码");
            return;
        }
        simulate("D:\\output_tmpfile_use.ntm");
    });

}

MainWindow::~MainWindow()
{
//    fclose(source);
    file.close();
    delete ui;
}
