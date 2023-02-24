/*********************************/
/*UTIL.cpp                       */
/*版本：2.0                      */
/*作者：Liji Chan                */
/*********************************/
/*接口函数：treePrint等若干      */
/*功能：提供一系列功能函数       */
/*********************************/

#include<iostream>
#include<stdio.h>
#include "GLOBAL.h"
#include "UTIL.h"
#include <QByteArray>
#include<QString>
#include<QMessageBox>
#include<QLatin1String>
#include <string.h>

using namespace std;
char* tempstr;
extern QString tokenStr;
extern QString prtStr;
//extern void parPnt(QString ar);//打印句子到界面相应区域
extern void trePnt(QString ar);

/* token打印函数*/
void printToken(TokenType tokenType, const char* tokenString) {
    switch (tokenType) {
    case IF:
    case ELSE:
    case INT:
    case FLOAT:
    case RETURN:
    case VOID:
    case DO:
    case WHILE:
        printf("reserved word: %s\n", tokenString);
        prtStr="reserved word: %s\n"+QString( tokenString);tokenStr=QString(tokenString);
        break;
    case EQ:printf("==\n");prtStr=("==\n");tokenStr="==\n"; break;
    case UNEQ:printf("!=\n");prtStr=("!=\n");tokenStr="!=\n"; break;
    case ASSIGN:printf("=\n");prtStr=("=\n");tokenStr="=\n"; break;
    case LT:printf("<\n");prtStr=("<\n"); tokenStr="<\n"; break;
    case LT_EQ:printf("<=\n"); prtStr=("<=\n"); tokenStr="<=\n";break;
    case RT:printf(">\n"); prtStr=(">\n"); tokenStr=">\n";break;
    case RT_EQ:printf(">=\n");prtStr=(">=\n"); tokenStr=">=\n"; break;
    case PLUS:printf("+\n");prtStr=("+\n"); tokenStr="+\n"; break;
    case MINUS:printf("-\n");prtStr=("-\n"); tokenStr="-\n"; break;
    case MULTI:printf("*\n");prtStr=("*\n"); tokenStr="*\n"; break;
    case DIVI:printf("/\n");prtStr=("/\n"); tokenStr="/\n"; break;
    case MOD:printf("%\n"); prtStr=("%\n");tokenStr="%\n"; break;
    case LPAREN_1:printf("{\n");prtStr=("{\n"); tokenStr="{\n"; break;
    case RPAREN_1:printf("}\n");prtStr=("}\n");tokenStr="}\n";  break;
    case LPAREN_2:printf("[\n");prtStr=("[\n");tokenStr="[\n";  break;
    case RPAREN_2:printf("]\n");prtStr=("]\n");tokenStr="]\n";  break;
    case LPAREN_3:printf("(\n");prtStr=("(\n");tokenStr="(\n";  break;
    case RPAREN_3:printf(")\n");prtStr=(")\n");tokenStr=")\n";  break;
    case SEMI:printf(";\n");prtStr=(";\n");tokenStr=";\n"; break;
    case COMM:printf(",\n");prtStr=(",\n"); tokenStr=",\n"; break;
    case ID:printf("ID, name= %s\n", tokenString);
        prtStr="ID, name= "+QString( tokenString)+"\n"; tokenStr=QString(tokenString);
        break;
    case NUM_INT:
    case NUM_FLOAT:printf("NUM, value= %s\n", tokenString);
        prtStr="NUM, value= "+QString( tokenString)+"\n"; tokenStr=QString(tokenString);break;
    case ERROR:printf("ERROR: %s\n", tokenString);
        prtStr="ERROR: "+QString(tokenString)+"\n" ;tokenStr=QString(tokenString);break;
    case ENDFILE:printf("EOF\n");prtStr=("EOF\n"); tokenStr="EOF\n"; break;
    default:/* 理论上不会发生*/
        printf("Unknown token: %d\n", tokenType);
        prtStr=("Unknown token: "+QString::number(tokenType)+"\n");tokenStr=QString(tokenString);
        break;
    }
}

/* 生成特定的表达式结点*/
TreeNode* newExpNode(ExpKind kind) {
    TreeNode* t = (TreeNode*)malloc(sizeof(TreeNode));
    if (t == NULL) {
        printf("Out of memory error at line %d\n", lineno);
        QString pntstc = "Out of memory error at line " + QString::number(lineno);
        QMessageBox::about(NULL,"错误",pntstc);
    }else {
        for (int i = 0; i < MAXCHILDREN; i++) {
            t->children[i] = NULL;
        }
        t->sibling = NULL;
        t->nodeKind = ExpK;
        t->kind.exp = kind;
        t->lineno = lineno;
        t->type = Void;
    }
    return t;
}

/* 生成特定的语句结点*/
TreeNode* newStmtNode(StmtKind kind) {
    TreeNode* t = (TreeNode*)malloc(sizeof(TreeNode));
    if (t == NULL) {
        printf("Out of memory error at line %d\n", lineno);
        QString pntstc = "Out of memory error at line " + QString::number(lineno);
        QMessageBox::about(NULL,"错误",pntstc);
    }else {
        for (int i = 0; i < MAXCHILDREN; i++) {
            t->children[i] = NULL;
        }
        t->sibling = NULL;
        t->nodeKind = StmtK;
        t->kind.stmt = kind;
        t->lineno = lineno;
    }
    return t;
}

/* 对一个字符数组进行拷贝*/
char* copyString(const char* s){
    int n;
    char* t;
    if (s == NULL) return NULL;
    n = strlen(s) + 1;
    t = (char*) malloc(n);
    if (t == NULL){
        printf("Out of memory error at line %d\n", lineno);
        QString pntstc = "Out of memory error at line " + QString::number(lineno);
        QMessageBox::about(NULL,"错误",pntstc);
    }

    else strcpy(t, s);
    return t;
}

/* 返回运算符类型对应的字符串*/
static char* getOpKind(TokenType token) {
    char* str = NULL;
    switch (token) {
    case EQ:str = new char[3]; strcpy(str,"=="); break;
    case UNEQ:str = new char[3];strcpy(str,"!=") ; break;
    case ASSIGN:str = new char[2];strcpy(str,"=") ; break;
    case LT:str = new char[2];strcpy(str,"<"); break;
    case LT_EQ:str = new char[3];strcpy(str,"<="); break;
    case RT:str = new char[3];strcpy(str,"<="); break;
    case RT_EQ:str = new char[3];strcpy(str,">=") ; break;
    case PLUS:str =new char[2];strcpy(str,"+"); break;
    case MINUS:str = new char[2];strcpy(str,"-"); break;
    case MULTI:str = new char[2];strcpy(str,"*"); break;
    case DIVI:str =new char[2];strcpy(str,"/") ; break;
    case MOD:str =new char[2];strcpy(str,"%"); break;
    default:
        printf("Unexpected operator token at line %d\n", lineno);
        QMessageBox::about(NULL,"提示","Unexpected operator token at line "+QString::number(lineno));
        break;
    }
    return str;
}

/* 返回结点类型对应的字符串*/
static char* getNodeString(TreeNode* t) {
    char* str = NULL;
    if (t->nodeKind == StmtK) {
        /* 语句结点*/
        switch (t->kind.stmt) {
        case DefId:str=new char[6];strcpy(str,"DefId") ; break;
        case DefFun:str=new char[7];strcpy(str,"DefFun"); break;
        case ParmK:str=new char[10];strcpy(str,"Parmeters") ; break;
        case IfK:str=new char[3];strcpy(str,"If") ; break;
        case ExpT:str=new char[4];strcpy(str,"Exp"); break;
        case RetK:str=new char[7];strcpy(str,"Return") ; break;
        case WhileK:str=new char[9];strcpy(str,"do-While") ; break;
        case CompK:str=new char[9];strcpy(str,"Compound") ; break;
        case CallK:str=new char[5];strcpy(str,"Call") ; break;
        default:
            /* 理论上不会出现*/
            printf("\nFailed to recognize the StmtKind at line %d\n", t->lineno);
//            QMessageBox::about(NULL,"提示","\nFailed to recognize the StmtKind at line "+QString::number(t->lineno) );
            trePnt("Failed to recognize the StmtKind at line "+QString::number(t->lineno));
            break;
        }
    }else if(t->nodeKind == ExpK){
        /* 表达式结点*/
        switch (t->kind.exp) {
        case OpK:
            str = getOpKind(t->attr.op);
            break;
        case IntK:
            str = new char[24];
            sprintf(str, "%d", t->attr._int);
            break;
        case FloatK:
            str = new char[24];
            sprintf(str, "%f", t->attr._float);
            break;
        case IdK:
            if(t->type != Void){
                /* type属性不为Void说明该结点属于某个变量定义语句*/
                str = new char[50];
                char* type=NULL;
                switch (t->type) {
                case Int:
                    type = new char[4];strcpy(type,"int");
                    break;
                case Float:
                    type = new char[6];strcpy(type,"float");
                    break;
                case IntArr:
                    type = new char[9];strcpy(type,"intArray") ;
                    break;
                case FloArr:
                    type = new char[11];strcpy(type,"floatArray");
                    break;
                }
                if(type!=NULL)
                sprintf(str, "%s(%s)", t->attr.name, type);
                delete[] type;
            }else {
                /* type属性为Void说明该结点属于某个表达式语句*/
                str = copyString(t->attr.name);
            }
            break;
        default:
            /* 理论上不会出现*/
            printf("\nFailed to recognize the ExpKind at line %d\n", t->lineno);
            QMessageBox::about(NULL,"提示","\nFailed to recognize the ExpKind at line "+QString::number(t->lineno));
            break;
        }
    }else {
        /* 理论上不会出现*/
        printf("Unexpected node kind at line %d\n", t->lineno);
        QMessageBox::about(NULL,"提示","Unexpected node kind at line "+QString::number(t->lineno));
    }
    return str;
}

/* 子递归函数*/
static void nodePrint(TreeNode* t, int level) {
    if (t != NULL) {
        for (int i = 1; i <= level; i++) {
            printf("\t");
            trePnt("\t");
        }
        char* str = getNodeString(t);
        printf("|- %s\n", str);
        QString temqs=QString(QLatin1String(str));
        trePnt("|- ");
        trePnt(temqs);
        trePnt("\n");
        delete[] str;
        for (int i = 0; i < MAXCHILDREN; i++) {
            nodePrint(t->children[i], level + 1);
        }
        if (t->sibling != NULL) {
            nodePrint(t->sibling, level);
        }
    }
}


/* 打印语法树*/
void treePrint(TreeNode* t) {
    /* 控制台横向打印，深度优先*/
    printf("START\n");
    trePnt("START\n");
    nodePrint(t, 0);
}


/* 拷贝EnterList链表*/
EnterList copyEnList(EnterList enList) {
    if (enList == NULL) {
        return NULL;
    }
    EnterList newEL = (EnterList)malloc(sizeof(Enter));
    if (newEL != NULL) {
        newEL->father = NULL;
        newEL->brother = NULL;
        newEL->childLink = NULL;
        newEL->funcName = enList->funcName;
        newEL->table = enList->table;
        /* 复制子链*/
        EnterList p, q, child;
        p = enList->childLink;
        while (p != NULL) {
            child = copyEnList(p);
            child->father = newEL;
            if (newEL->childLink == NULL) {
                newEL->childLink = child;
            }
            else {
                q = newEL->childLink;
                while (q->brother != NULL) {
                    q = q->brother;
                }
                q->brother = child;
            }
            p = p->brother;
        }
    }
    return newEL;
}
