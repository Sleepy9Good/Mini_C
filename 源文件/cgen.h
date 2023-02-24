#pragma once
#ifndef _CGEN_H
#define _CGEN_H
#include "ANALYSE.h"
#include "CGEN.h"
#include "CODE.h"
#include "UTIL.h"
#include <stack>
#include<QString>

extern int tmpOffset; /* 指示临时变量存储边界的指针*/
extern std::stack<char*> callStack; /* 函数调用栈*/


/* 生成中间代码的主接口*/
void codeGen(TreeNode* syntaxTree, const char* codefile);
#endif
