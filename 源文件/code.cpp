/*************************************/
/*CODE.cpp                           */
/*版本：1.0                          */
/*作者：Liji Chan                    */
/*************************************/
/*接口函数：emitRO()等               */
/*功能：生成相应的中间代码或注释     */
/*************************************/

#include "GLOBAL.h"
#include "CODE.h"
#include<QString>

extern void codPnt(QString ar);

/* 当前指令任务在虚拟机执行时的位置索引*/
int emitLoc = 0;

/* 记录当前出现过的最高的位置索引*/
int highEmitLoc = 0;

/* 生成注释*/
void emitComment(const char* c) {
    fprintf(output, "* %s\n", c);
    QString tqstr = "* "+QString(c)+"\n";
    codPnt(tqstr);
}

/* 返回当前的索引位置，并跳过howMany个位置*/
int emitSkip(int howMany) { //输出从控制台改成文件
    int i = emitLoc;
    emitLoc += howMany;
    highEmitLoc = (highEmitLoc < emitLoc) ? emitLoc : highEmitLoc;
    return i;
}

/* 跳转到之前的某个索引位置*/
void emitBackup(int loc) {
    if (loc > highEmitLoc) emitComment("BUG in emitBackup");
    emitLoc = loc;
}

/* 跳转到当前最高的索引位置*/
void emitRestore() {
    emitLoc = highEmitLoc;
}

/* 生成寄存器操作类型指令
 * op = 操作码
 * r = 目标寄存器
 * s = 源寄存器1
 * t = 源寄存器2
 * c = 注释
 */
void emitRO(const char* op, int r, int s, int t, const char* c) {
    fprintf(output, "%3d:  %5s  %d,%d,%d \t%s\n", emitLoc++, op, r, s, t, c);
    QString tqstr=QString("%1:  %2  %3,%4,%5 \t%6\n").arg(emitLoc-1,3,10,QLatin1Char(' ')).arg(QString(op),5,QLatin1Char(' ')).
            arg(r).arg(s).arg(t).arg(c);
    codPnt(tqstr);
    highEmitLoc = (highEmitLoc < emitLoc) ? emitLoc : highEmitLoc;
}

/* 生成“寄存器-内存”操作类型指令
 * op = 操作码
 * r = 目标寄存器
 * d = 相对偏移量
 * s = 基址寄存器
 * c = 注释
 */
void emitRM(const char* op, int r, int d, int s, const char* c) {
    fprintf(output, "%3d:  %5s  %d,%d(%d) \t%s\n", emitLoc++, op, r, d, s, c);
    QString tqstr=QString("%1:  %2  %3,%4(%5) \t%6\n").arg(emitLoc-1,3,10,QLatin1Char(' ')).arg(QString(op),5,QLatin1Char(' ')).
            arg(r).arg(d).arg(s).arg(c);
    codPnt(tqstr);
    highEmitLoc = (highEmitLoc < emitLoc) ? emitLoc : highEmitLoc;
}

/* 重载，参数d类型为 float*/
void emitRM(const char* op, int r, float d, int s, const char* c) {
    fprintf(output, "%3d:  %5s  %d,%f(%d) \t%s\n", emitLoc++, op, r, d, s, c);
    QString tqstr=QString("%1:  %2  %3,%4(%5) \t%6\n").arg(emitLoc-1,3,10,QLatin1Char(' ')).arg(QString(op),5,QLatin1Char(' ')).
            arg(r).arg(d).arg(s).arg(c);
    codPnt(tqstr);
    highEmitLoc = (highEmitLoc < emitLoc) ? emitLoc : highEmitLoc;
}

/* 生成“寄存器-内存”操作类型指令（绝对地址寻址）
 * op = 操作码
 * r = 目标寄存器
 * a = 绝对地址
 * c = 注释
 */
void emitRM_Abs(const char* op, int r, int a, const char* c) {
    fprintf(output, "%3d:  %5s  %d,%d(%d) \t%s\n",
        emitLoc, op, r, a - (emitLoc + 1), pc, c);
    QString tqstr=QString("%1:  %2  %3,%4(%5) \t%6\n").arg(emitLoc,3,10,QLatin1Char(' ')).arg(QString(op),5,QLatin1Char(' ')).
            arg(r).arg(a - (emitLoc + 1)).arg(pc).arg(c);
    codPnt(tqstr);
    ++emitLoc;
    highEmitLoc = (highEmitLoc < emitLoc) ? emitLoc : highEmitLoc;
}
