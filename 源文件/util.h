#pragma once
#ifndef _UTIL_H_
#define _UTIL_H_
#include"GLOBAL.h"

/* token打印函数*/
void printToken(TokenType tokenType, const char* tokenString);

/* 对一个字符数组进行拷贝*/
char* copyString(const char* s);

/* 打印语法树*/
void treePrint(TreeNode* t);

/* 生成特定的表达式结点*/
TreeNode* newExpNode(ExpKind kind);

/* 生成特定的语句结点*/
TreeNode* newStmtNode(StmtKind kind);

/* 拷贝EnterList链表*/
EnterList copyEnList(EnterList enList);
#endif // !_UTIL_H_

