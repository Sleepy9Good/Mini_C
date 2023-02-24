#pragma once
#ifndef _ANALYSE_H
#define _ANALYSE_H
#include "SYMTAB.h"


/* 生成符号表*/
void buildSymtab(TreeNode* syntaxTree, TokenTable hashTable);

/* 存放函数最大内存偏移量的链表项*/
typedef struct FuncOffset{
    char* funcName;
    int offs; //偏移量
    FuncOffset* next;
} * FuncOffList;

extern EnterList funcEnList[];

extern FuncOffList offsetArr[];

extern const int MAX_FUN;

extern int level;

extern TokenTable tabList[];
/* 记录当前所检查到的函数的名字*/
extern char* curFuncName;

/* 记录当前所检查到的函数的返回值类型*/
extern ExpType returnType;


#endif
