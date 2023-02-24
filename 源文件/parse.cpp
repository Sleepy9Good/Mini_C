/*********************************/
/*PARSE.cpp                      */
/*版本：2.0                      */
/*作者：Liji Chan                */
/*接口函数：parse()              */
/*功能：对源文件进行语法分析，   */
/*      生成并打印语法树。       */
/*********************************/

#include <iostream>
#include<QMessageBox>
#include<QString>
#include "GLOBAL.h"
#include "PARSE.h"
#include "SCAN.h"
#include "UTIL.h"

using namespace std;
QString tokenStr;//记录符号
QString prtStr;//记录句子
//extern void parPnt(QString ar);//打印句子到界面相应区域
extern void trePnt(QString ar);

/* 生成语法树的递归子程序函数如下*/
static TreeNode* definition_list();
static TreeNode* definition();
static TreeNode* array_definition();
static TreeNode* function_definition(TreeNode*);
static TreeNode* parameters();
static TreeNode* parameter();
static TreeNode* compound_stmt();
static TreeNode* local_definitions();
static TreeNode* statement_list();
static TreeNode* statement();
static TreeNode* input_stmt();
static TreeNode* output_stmt();
static TreeNode* expression_stmt();
static TreeNode* condition_stmt();
static TreeNode* dowhile_stmt();
static TreeNode* return_stmt();
static TreeNode* expression();
static TreeNode* simple_expression(bool&, bool&, bool&, bool&);
static TreeNode* additive_expression(bool&, bool&, bool&);
static TreeNode* term(bool&, bool&);
static TreeNode* factor(bool&);
static TreeNode* arguments();

/* 保存当前单词的token类型*/
static TokenType token;

/* 错误处理函数*/
inline static void SyntaxError(const char* message) {
    printf("Syntax error at line %d: %s", lineno, message);
    QString fau = "Syntax error at line ";
        fau = fau + QString::number(lineno-12) + ": " + QString(message) + tokenStr;
        QMessageBox::about(NULL,"错误",fau);return;
}

/* 匹配当前token是否符合语法*/
static void match(TokenType expected) {
    if (token == expected) {
        /* 匹配成功则读取下一个token*/
        token = getToken();
    }else {
        printToken(token, TokenString);
        SyntaxError("Unexpected token -> ");
    }
}

TreeNode* definition_list() {
    TreeNode* t, * p;
    p = t = definition();
    while(token != ENDFILE && token != ERROR){
        if (t == NULL) {
            p = t = definition();
        }else {
            p->sibling = definition();
            if(p->sibling != NULL){
                p = p->sibling;
            }
        }
    }
    return t;
}

TreeNode* definition() {
    TreeNode* t = NULL;
    ExpType expt;
    char* name=NULL;
    switch (token) {
    case INT:
    case FLOAT:
        /* 这里需要预读取标识符的后面一个token来判断是否为函数或者数组，
           因此用expt和name临时保存数据类型和变量名*/
        if (token == INT) {
            expt = Int;
        }else {
            expt = Float;
        }
        match(token);
        if (token == ID) {
            name = copyString(TokenString);
        }
        match(ID);
        /* 按基本类型、数组和函数分别生成相应的语法树结点*/
        if (token == LPAREN_3) {
            /* 函数*/
            TreeNode* defFun = newStmtNode(DefFun);
            if (defFun != NULL) {
                /* 函数结点有三个子节点，其中IdK结点存储返回值类型和函数名，
                   ParmK结点存储参数列表，CompK结点存储函数要执行的语句*/
                TreeNode* idK = newExpNode(IdK);
                if (idK != NULL) {
                    idK->type = expt;
                    idK->attr.name = name;
                    defFun->children[0] = idK;
                    defFun = function_definition(defFun);
                    t = defFun;
                }
            }
        }else {
            TreeNode* defId = newStmtNode(DefId);
            if (defId != NULL) {
                TreeNode* check = array_definition();
                TreeNode* idK = newExpNode(IdK);
                if (idK != NULL) {
                    if (check == NULL) {
                        /* check为空，说明是基本类型*/
                        idK->type = expt;
                    }
                    else {
                        /* check不为空，说明是数组*/
                        idK->type = (expt == Int) ? IntArr : FloArr;
                        /* 数组结点有一个子节点，存放该数组的长度*/
                        idK->children[0] = check;
                    }
                    idK->attr.name = name;
                    defId->children[0] = idK;
                    t = defId;
                }
                match(SEMI);
            }
        }
        break;
    case VOID: {
        /* 函数返回的数据类型可以是void，而变量不可以*/
        TreeNode* defFun = newStmtNode(DefFun);
        if (defFun != NULL) {
            TreeNode* idK = newExpNode(IdK);
            if (idK != NULL) {
                idK->type = Void;
                match(VOID);
                if (token == ID) {
                    idK->attr.name = copyString(TokenString);
                }
                match(ID);
                defFun->children[0] = idK;
                defFun = function_definition(defFun);
                t = defFun;
            }
        }
        break;
    }
    default:
        printToken(token, TokenString);
        SyntaxError("Unexpected token -> ");
        token = getToken();
        break;
    }
    return t;
}

TreeNode* array_definition() {
    /* 进行数组匹配*/
    TreeNode* t = NULL;
    if (token == LPAREN_2) {
        match(LPAREN_2);
        t = expression();
        match(RPAREN_2);
    }
    return t;
}

TreeNode* function_definition(TreeNode* defFun) {
    /* 进行函数匹配*/
    match(LPAREN_3);
    defFun->children[1] = parameters();
    match(RPAREN_3);
    defFun->children[2] = compound_stmt();
    return defFun;
}

TreeNode* parameters() {
    TreeNode *param, *t, *p;
    param = newStmtNode(ParmK);
    p = t = NULL;
    if (param != NULL) {
        if (token == VOID) {
            /* void说明参数列表为空*/
            match(VOID);
        }else if (token == INT || token == FLOAT) {
            /* 参数用sibling链表的形式连接*/
            p = t = parameter();
            while (token == COMM) {
                match(COMM);
                if (t == NULL) {
                    p = t = parameter();
                }else {
                    p->sibling = parameter();
                    if (p->sibling != NULL) {
                        p = p->sibling;
                    }
                }
            }
        }else {
            printToken(token, TokenString);
            SyntaxError("Unexpected token -> ");
        }
        /* ParmK结点的第一个子节点存放该参数链表的头节点*/
        param->children[0] = t;
    }
    return param;
}

TreeNode* parameter() {
    TreeNode* t = NULL;
    ExpType expt;
    char* name=NULL;
    if (token == INT || token == FLOAT) {
        /* 这里需要预读取标识符的后面一个token来判断是否为数组，
           因此用expt和name临时保存数据类型和变量名*/
        if (token == INT) {
            expt = Int;
        }else {
            expt = Float;
        }
        match(token);
        if (token == ID) {
            name = copyString(TokenString);
        }
        match(ID);
        /* 按基本类型、数组分别生成相应的语法树结点*/
        TreeNode* idK = newExpNode(IdK);
        if (idK != NULL) {
            idK->attr.name = name;
            t = idK;
            if (token == LPAREN_2) {
                /* 数组*/
                idK->type = (expt == Int) ? IntArr : FloArr;
                match(LPAREN_2);
                match(RPAREN_2);
            }else {
                /* 基本类型*/
                idK->type = expt;
            }
        }
    }else {
        printToken(token, TokenString);
        SyntaxError("Unexpected token -> ");
    }
    return t;
}

TreeNode* compound_stmt() {
    TreeNode* t = newStmtNode(CompK);
    if (t != NULL) {
        match(LPAREN_1);
        /* CompK结点有两个子节点，局部变量定义和语句组*/
        t->children[0] = local_definitions();
        t->children[1] = statement_list();
        match(RPAREN_1);
    }
    return t;
}

TreeNode* local_definitions() {
    TreeNode* t, * p;
    ExpType expt;
    char* name=NULL;
    p = t = NULL;
    /* 局部变量类型不能为void*/
    while(token == INT || token == FLOAT) {
        /* 局部变量定义是一个sibling链表*/
        if (t == NULL) {
            p = t = newStmtNode(DefId);
        }else {
            p->sibling = newStmtNode(DefId);
            if(p->sibling != NULL){
                p = p->sibling;
            }
        }
        if (p != NULL) {
            /* 这里需要预读取标识符的后面一个token来判断是否为数组，
               因此用expt和name临时保存数据类型和变量名*/
            if (token == INT) {
                expt = Int;
            }else {
                expt = Float;
            }
            match(token);
            if (token == ID) {
                name = copyString(TokenString);
            }
            match(ID);
            TreeNode* check = array_definition();
            TreeNode* idK = newExpNode(IdK);
            if (idK != NULL) {
                if (check == NULL) {
                    /* check为空，说明是基本类型*/
                    idK->type = expt;
                }
                else {
                    /* check不为空，说明是数组*/
                    idK->type = (expt == Int) ? IntArr : FloArr;
                    /* 数组结点有一个子节点，存放该数组的长度*/
                    idK->children[0] = check;
                }
                idK->attr.name = name;
                p->children[0] = idK;
            }
            match(SEMI);
        }
    }
    return t;
}

TreeNode* statement_list() {
    TreeNode *t, *p;
    p = t = NULL;
    /* 当token属于statement的first集合时进行匹配*/
    while (token == SEMI || token == IF || token == DO || token == ID
        || token == LPAREN_1 || token == RETURN || token == INPUT || token == OUTPUT
        || token == LPAREN_3 || token == NUM_INT || token == NUM_FLOAT) {
        /* 语句组也是一个sibling链表*/
        if (t == NULL) {
            p = t = statement();
        }else {
            p->sibling = statement();
            if(p->sibling != NULL){
                p = p->sibling;
            }
        }
    }
    return t;
}

TreeNode* statement() {
    TreeNode* t = NULL;
    switch (token) {
    case INPUT: t = input_stmt(); break;
    case OUTPUT: t = output_stmt(); break;
    case IF:t = condition_stmt(); break;
    case DO:t = dowhile_stmt(); break;
    case RETURN:t = return_stmt(); break;
    case LPAREN_1:t = compound_stmt(); break;
    case SEMI:
    case ID:
    case LPAREN_3:
    case NUM_INT:
    case NUM_FLOAT:t = expression_stmt(); break;
    default:
        printToken(token, TokenString);
        SyntaxError("Unexpected token -> ");
        break;
    }
    return t;
}

TreeNode* input_stmt() {
    TreeNode* t = newStmtNode(InK);
    match(INPUT);
    t->children[0] = expression();
    match(SEMI);
    return t;
}

TreeNode* output_stmt() {
    TreeNode* t = newStmtNode(OutK);
    match(OUTPUT);
    t->children[0] = expression();
    match(SEMI);
    return t;
}

TreeNode* condition_stmt() {
    /* IfK最多有三个子节点，进行判断的表达式、
       条件成立时执行的语句（和条件不成立时执行的语句）*/
    TreeNode* t = newStmtNode(IfK);
    if (t != NULL) {
        match(IF);
        match(LPAREN_3);
        t->children[0] = expression();
        match(RPAREN_3);
        t->children[1] = statement();
        if (token == ELSE) {
            /* else必须和最近的一个if匹配*/
            match(ELSE);
            t->children[2] = statement();
        }
    }
    return t;
}

TreeNode* dowhile_stmt() {
    /* WhileK有两个子节点，循环条件成立时
       执行的语句和进行循环条件判断的表达式*/
    TreeNode* t = newStmtNode(WhileK);
    if (t != NULL) {
        match(DO);
        t->children[0] = statement();
        match(WHILE);
        match(LPAREN_3);
        t->children[1] = expression();
        match(RPAREN_3);
        match(SEMI);
    }
    return t;
}

TreeNode* return_stmt() {
    TreeNode* t = newStmtNode(RetK);
    match(RETURN);
    if (token == SEMI) {
        /* 返回值可为空*/
        match(SEMI);
    }else {
        if (t != NULL) {
            t->children[0] = expression();
            match(SEMI);
        }
    }
    return t;
}

TreeNode* expression_stmt() {
    TreeNode* t = newStmtNode(ExpT);
    if (token == SEMI) {
        /* 表达式可为空*/
        match(SEMI);
    }else {
        if (t != NULL) {
            t->children[0] = expression();
            match(SEMI);
        }
    }
    return t;
}

TreeNode* expression() { //下一步：解决ExpT无内容问题
    TreeNode* t = NULL;
    if (token == ID || token == LPAREN_3 || token == NUM_INT || token == NUM_FLOAT) {
        bool first_addi = true;
        bool first_term = true;
        bool first_factor = true;
        bool is_vari = false;
        t = simple_expression(first_addi, first_term, first_factor, is_vari);
        if(token == ASSIGN && first_addi && first_term && first_factor && is_vari){
            /* 以上条件成立时，说明第一个simple expression是一个基本类型变量
               或者数组中的某个项，此时语句是一个赋值语句，注意赋值语句是可嵌套的*/
            TreeNode* p = newExpNode(OpK);
            if (p != NULL) {
                p->children[0] = t;
                p->attr.op = token;
                t = p;
                match(ASSIGN);
                t->children[1] = expression();
            }
        }
    }else {
        printToken(token, TokenString);
        SyntaxError("Unexpected token -> ");
    }
    return t;
}

TreeNode* simple_expression(bool& first_addi, bool& first_term, bool& first_factor, bool& is_vari) {
    /* 采用引用传递的方式使得标记在匹配过程中可以实时进行监控，
       任一标记不成立，该simple expression都不可进行赋值*/
    TreeNode* t = additive_expression(first_term, first_factor, is_vari);
    if(token == LT || token == LT_EQ || token == RT
        || token == RT_EQ || token == EQ || token == UNEQ) {
        /* 当token是比较运算符时，该simple expression是一个比较表达式*/
        TreeNode* p = newExpNode(OpK);
        if (p != NULL) {
            p->children[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            t->children[1] = additive_expression(first_term, first_factor, is_vari);
            first_addi = false;
        }
    }
    return t;
}

TreeNode* additive_expression(bool& first_term, bool& first_factor, bool& is_vari) {
    TreeNode* t = term(first_factor, is_vari);
    while (token == PLUS || token == MINUS) {
        /* +、-是左结合运算*/
        TreeNode* p = newExpNode(OpK);
        if (p != NULL) {
            p->children[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            t->children[1] = term(first_factor, is_vari);
            first_term = false;
        }
    }
    return t;
}

TreeNode* term(bool& first_factor, bool& is_vari) {
    TreeNode* t = factor(is_vari);
    while (token == MULTI || token == DIVI || token == MOD) {
        /* /、*、%是左结合运算*/
        TreeNode* p = newExpNode(OpK);
        if (p != NULL) {
            p->children[0] = t;
            p->attr.op = token;
            t = p;
            match(token);
            t->children[1] = factor(is_vari);
            first_factor = false;
        }
    }
    return t;
}

TreeNode* factor(bool& is_vari) {
    TreeNode* t = NULL;
    switch (token) {
        case NUM_INT: {
            /* 整数常量*/
            TreeNode* intK = newExpNode(IntK);
            if (intK != NULL) {
                intK->attr._int = atoi(TokenString);
                t = intK;
                match(token);
            }
            break;
        }
        case NUM_FLOAT: {
            /* 浮点数常量*/
            TreeNode* floatK = newExpNode(FloatK);
            if (floatK != NULL) {
                floatK->attr._float = atof(TokenString);
                t = floatK;
                match(token);
            }
            break;
        }
        case LPAREN_3:
            /* 括号*/
            match(LPAREN_3);
            t = expression();
            match(RPAREN_3);
            break;
        case ID:{
            /* 这里需要预读取标识符的后面一个token来判断是否为函数调用或者数组访问，
               因此用name临时保存变量名*/
            char* name = copyString(TokenString);
            match(ID);
            if (token == LPAREN_3) {
                /* 函数调用*/
                TreeNode* callK = newStmtNode(CallK);
                if (callK != NULL) {
                    /* CallK结点有两个子节点，一个是函数名，一个是参数列表*/
                    TreeNode* idK = newExpNode(IdK);
                    if (idK != NULL) {
                        idK->attr.name = name;
                        callK->children[0] = idK;
                        match(LPAREN_3);
                        callK->children[1] = arguments();
                        match(RPAREN_3);
                        t = callK;
                    }
                }
            }
            else {
                /* 变量*/
                TreeNode* check = array_definition();
                TreeNode* idK = newExpNode(IdK);
                if (idK != NULL) {
                    /* check不为空，说明是数组的某个项*/
                    if (check != NULL) {
                        /* 数组结点有一个子节点，存放该数组项的索引*/
                        idK->children[0] = check;
                    }
                    idK->attr.name = name;
                    t = idK;
                    is_vari = true;
                }
            }
            break;
        }
        default:
        printToken(token, TokenString);
            SyntaxError("Unexpected token -> ");
            break;
    }
    return t;
}

TreeNode* arguments() {
    TreeNode* t, * p;
    p = t = NULL;
    if (token == ID || token == LPAREN_3 || token == NUM_INT || token == NUM_FLOAT) {
        p = t = expression();
        while (token == COMM) {
            match(COMM);
            if (token == ID || token == LPAREN_3 || token == NUM_INT || token == NUM_FLOAT) {
                /* 参数用sibling链表的方式存储*/
                if (t == NULL) {
                    p = t = expression();
                }else{
                    p->sibling = expression();
                    if(p->sibling != NULL){
                        p = p->sibling;
                    }
                }
            }else {
                printToken(token, TokenString);
                SyntaxError("Unexpected token -> ");
                break;
            }
        }
    }
    return t;
}

/* 主接口函数*/
TreeNode* parse() {
    token = getToken();
    TreeNode* t = definition_list();
    treePrint(t);
    return t;
}
