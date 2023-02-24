#include "mainwindow.h"
#include <QApplication>

#include"global.h"
#include"parse.h"
#include"scan.h"
#include "parse.h"
#include "analyse.h"
#include "cgen.h"
#include "tm.h"
#include<time.h>
#include<iostream>


using namespace std;

FILE* source;/* 源文件流*/
FILE* output;/* 输出文件流*/


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
