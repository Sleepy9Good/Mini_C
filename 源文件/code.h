#pragma once
#ifndef _CODE_H
#define _CODE_H

extern int emitLoc;

/* 记录当前出现过的最高的位置索引*/
extern int highEmitLoc;



/* pc = "program counter" 程序计数器
 * 存放下一条指令的索引
 */
#define  pc 7

/* mp = "memory pointer" 内存指示器
 * 指向内存的最高地址
 */
#define  mp 6

/* cp = "curent pointer" 当前地址指示器
  * 指向内存中下一个可使用的单元地址
  */
#define cp 5

/* accumulator 累加器 */
#define  ac 0

/* 2nd accumulator 第二累加器 */
#define  ac1 1


/*********************************/
/*                               */
/*    中 间 代 码 生 成 接 口    */
/*                               */
/*********************************/

/* 生成注释*/
void emitComment(const char* c);

/* 返回当前的索引位置，并跳过howMany个位置*/
int emitSkip(int howMany);

/* 跳转到之前的某个索引位置*/
void emitBackup(int loc);

/* 跳转到当前最高的索引位置*/
void emitRestore();

/* 生成寄存器操作类型指令*/
void emitRO(const char* op, int r, int s, int t, const char* c);

/* 生成“寄存器-内存”操作类型指令*/
void emitRM(const char* op, int r, float d, int s, const char* c);

/* 重载函数*/
void emitRM(const char* op, int r, int d, int s, const char* c);

/* 生成“寄存器-内存”操作类型指令（绝对地址寻址）*/
void emitRM_Abs(const char* op, int r, int a, const char* c);

#endif
