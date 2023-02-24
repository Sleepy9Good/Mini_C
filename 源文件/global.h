#pragma once
#ifndef _GLOBAL_H_
#define _GLOBAL_H_
#include<stdio.h>

/* Token标记的类型*/
enum TokenType {
    /* 特殊标记*/
    ENDFILE, ERROR,
    /* 保留字标记*/
    IF, ELSE, INT, FLOAT, RETURN, VOID, DO, WHILE, INPUT, OUTPUT,
    /* 标识符、整型和浮点数标记*/
    ID, NUM_INT, NUM_FLOAT,
    /* 运算符标记*/
    EQ, UNEQ, ASSIGN, LT, LT_EQ, RT, RT_EQ, PLUS, MINUS, MULTI, DIVI, MOD,
    /* 括号分三级，1-3依次为{}、[]、()*/
    LPAREN_1, LPAREN_2, LPAREN_3, RPAREN_1, RPAREN_2, RPAREN_3,
    /* 分号，逗号*/
    SEMI, COMM
};

/* 语法树结点类型*/
enum NodeKind {
    StmtK, ExpK
};

/* 语句结点的具体类型*/
enum StmtKind {
    DefId, DefFun, ParmK, IfK, ExpT, RetK, WhileK, CompK, CallK, InK, OutK
};

/* 表达式结点的具体类型*/
enum ExpKind {
    OpK, IntK, FloatK, IdK
};

/* 基本数据类型和其数组类型*/
enum ExpType {
    Void, Int, Float, IntArr, FloArr
};

const int MAXCHILDREN = 3; /* 子节点最大个数*/

struct TreeNode {
    TreeNode* children[MAXCHILDREN];
    TreeNode* sibling; /* 兄弟结点*/
    NodeKind nodeKind;
    int lineno;
    union { StmtKind stmt; ExpKind exp; } kind;
    union {TokenType op;
        int _int;
        float _float;
        char* name;} attr;
    ExpType type;
};

/* 用于存放变量出现过的行的索引*/
typedef struct LineListRec {
    int lineno;
    LineListRec* next;
} *LineList;

/* 用于存放函数需要的参数*/
typedef struct ParamListRec {
    char* name;
    ExpType type;
    ParamListRec* next;
} *ParamList;

/* 用于存放符号表的每一个变量的有关信息*/
typedef struct BucketListRec {
    char* name;
    int memloc;  //变量在内存中的相对当前位置的偏移量
    ExpType type;  //变量的数据类型或者函数的返回值类型
    bool isParm;  //标识是否为函数参数
    bool isLocal;  //是否是局部变量
    bool isFunc; //是否为函数
    struct TreeNode* params;  //函数需要的参数列表
    struct TreeNode* compK; // 函数的复合语句结点
    struct BucketListRec* next;
} *BucketList;

typedef BucketList* TokenTable; /* 声明符号表类型*/

/* 用于构建符号表入口链表的表项*/
typedef struct Enter {
    char* funcName;
    TokenTable table;
    Enter* father;
    Enter* childLink;
    Enter* brother;
}*EnterList;


extern FILE* source; /* 源文件流*/
extern FILE* output; /* 输出文件流*/
extern int lineno; /* 光标当前所在行*/

extern const int MAXTOKENLEN; /* 标识符最大长度*/
extern char TokenString[]; /* 存放token的数组*/

#endif // !_GLOBAL_H_

