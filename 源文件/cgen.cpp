/*************************************/
/*CGEN.cpp                           */
/*版本：1.0                          */
/*作者：Liji Chan                    */
/*************************************/
/*接口函数：codeGen()                */
/*功能：生成中间代码                 */
/*************************************/

#include "CGEN.h"
#include "CODE.h"
#include "UTIL.h"
#include <stack>
#include<QString>
extern void codPnt(QString ar);

static const int MAX_TABSIZE = 14; /* 最多同时存在的符号表层数*/
int tmpOffset = 0; /* 指示临时变量存储边界的指针*/
std::stack<char*> callStack; /* 函数调用栈*/

/* 函数提前声明*/
static bool cGen(TreeNode* tree, EnterList enList = NULL, TokenTable* tabList = NULL, int level = 0);

/* 生成计算变量在内存中的地址的代码*/
static bool cGenMemPos(TreeNode* idK, TokenTable* tabList, int level, bool checkParm) {
    BucketList l;
    int loc = st_lookup(idK->attr.name, tabList, level, l, false);
    /* 将变量的相对偏移量加载到ac中*/
    emitRM("LDC", ac, loc, 0, "op: load relative offset");
    if (l->isLocal) {
        /* 局部变量的绝对地址等于 cp + ac*/
        emitComment("variable is local");
        emitRO("ADD", ac, cp, ac, "op: culculate absolute offset");
        /* 如果是数组类型且为函数参数，则通过多一次访址操作直接获取原实参数组的绝对地址*/
        if (checkParm && l->isParm && (l->type == IntArr || l->type == FloArr)) {
            emitComment("parameter also array");
            emitRM("LD", ac, 0, ac, "op: load source array absolute offset ");
        }
    }
    else {
        /* 全局变量的绝对地址等于 0 + ac*/
        emitComment("variable is national, so absolute offset is in ac(0)");
        //emitRM("LDA", ac, 0, ac, "op: culculate left absolute offset");
    }
    TreeNode* index = idK->children[0];
    if (index != NULL) {
        /* 该变量是数组项*/
        emitComment("variable is an item in array");
        /* 将数组首地址存入临时变量区*/
        emitRM("ST_INT", ac, tmpOffset--, mp, "op: push array initial offset");
        /* 把索引的数值存入 ac*/
        cGen(index, NULL, tabList, level);
        /* 重新读取数组首地址到 ac1*/
        emitRM("LD", ac1, ++tmpOffset, mp, "op: load array initial offset");
        /* 相加计算出索引对应的相对内存偏移量*/
        emitRO("ADD", ac, ac1, ac, "op: culculate item relative offset");
    }
    /* 当变量为整个数组时返回true*/
    return (idK->type == IntArr || idK->type == FloArr) ? true : false;
}

/* 用于生成某个表达式结点相关操作的中间代码*/
static void genExp(TreeNode* tree, TokenTable* tabList, int level) {
    if (tree == NULL) {
        return;
    }
    int loc;
    TreeNode* p1, * p2;
    switch (tree->kind.exp) {
    case OpK:
        emitComment("-> Operate");
        p1 = tree->children[0];
        p2 = tree->children[1];
        if (tree->attr.op != ASSIGN) {
            /* 生成将左操作数存入 ac 的代码*/
            cGen(p1, NULL, tabList, level);
            /* 生成将左操作数从 ac 存入内存中的临时变量区的代码*/
            if (p1->type == Int) {
                emitRM("ST_INT", ac, tmpOffset--, mp, "op: push left operand");
            }
            else {
                emitRM("ST_FLO", ac, tmpOffset--, mp, "op: push left operand");
            }
            /* 生成将右操作数存入 ac 的代码*/
            cGen(p2, NULL, tabList, level);
            /* 生成将左操作数从内存中的临时变量区存入 ac1 的代码*/
            emitRM("LD", ac1, ++tmpOffset, mp, "op: load left operand");
        }
        switch (tree->attr.op) {
        case PLUS:
            emitRO("ADD", ac, ac1, ac, "op +");
            break;
        case MINUS:
            emitRO("SUB", ac, ac1, ac, "op -");
            break;
        case MULTI:
            emitRO("MUL", ac, ac1, ac, "op *");
            break;
        case DIVI:
            emitRO("DIV", ac, ac1, ac, "op /");
            break;
        case MOD:
            emitRO("MOD", ac, ac1, ac, "op %");
            break;
        case ASSIGN: {
            emitComment("-> Assign");
            /* 生成将右操作数存入 ac 的代码*/
            cGen(p2, NULL, tabList, level);
            if (p2->type == Int) {
                emitRM("ST_INT", ac, tmpOffset--, mp, "op: push right operand");
            }
            else {
                emitRM("ST_FLO", ac, tmpOffset--, mp, "op: push right operand");
            }
            emitComment("culculate left operand mem pos");
            /* 将赋值目标变量在内存中的地址置入ac中*/
            cGenMemPos(p1, tabList, level, true);
            emitRM("LD", ac1, ++tmpOffset, mp, "op: load right operand");
            if (p1->type == Int) {
                emitRM("ST_INT", ac1, 0, ac, "op: push right operand into target pos");
            }
            else {
                emitRM("ST_FLO", ac1, 0, ac, "op: push right operand into target pos");
            }
            emitComment("<- Assign");
            break;
        }
        default:
            switch (tree->attr.op) {
            case LT:
                emitRO("SUB", ac, ac1, ac, "op <");
                emitRM("JLT", ac, 2, pc, "br if true");
                break;
            case LT_EQ:
                emitRO("SUB", ac, ac1, ac, "op <=");
                emitRM("JLE", ac, 2, pc, "br if true");
                break;
            case RT:
                emitRO("SUB", ac, ac1, ac, "op >");
                emitRM("JGT", ac, 2, pc, "br if true");
                break;
            case RT_EQ:
                emitRO("SUB", ac, ac1, ac, "op >=");
                emitRM("JGE", ac, 2, pc, "br if true");
                break;
            case EQ:
                emitRO("SUB", ac, ac1, ac, "op ==");
                emitRM("JEQ", ac, 2, pc, "br if true");
                break;
            case UNEQ:
                emitRO("SUB", ac, ac1, ac, "op !=");
                emitRM("JNE", ac, 2, pc, "br if true");
                break;
            default:
                emitComment("BUG: Unknown operator");
                break;
            }
            /* 公共代码*/
            emitRM("LDC", ac, 0, ac, "false case");
            emitRM("LDA", pc, 1, pc, "unconditional jmp");
            emitRM("LDC", ac, 1, ac, "true case");
            break;
        }
        emitComment("<- Operate");
        break;
    case IntK: {
        emitComment("-> Const int");
        int val = tree->attr._int;
        emitRM("LDC", ac, val, 0, "load const int");
        emitComment("<- Const int");
        break;
    }
    case FloatK: {
        emitComment("-> Const float");
        float val = tree->attr._float;
        emitRM("LDC", ac, val, 0, "load const float");
        emitComment("<- Const float");
        break;
    }
    case IdK: {
        emitComment("-> Variable");
        cGenMemPos(tree, tabList, level, true);
        emitRM("LD", ac, 0, ac, "load variable");
        emitComment("<- Variable");
        break;
    }
    default:
        break;
    }
}

/* 用于生成某个语句结点相关操作的中间代码*/
static bool genStmt(TreeNode* tree, EnterList enList, TokenTable* tabList, int level) {
    switch (tree->kind.stmt) {
    case DefFun: {
        TreeNode* idK = tree->children[0];
        /* 程序从调用main函数开始运行*/
        if (strcmp(idK->attr.name, "main") == 0) {
            emitComment("-> main");
            TreeNode* comp = tree->children[2];
            /* 将全局变量的内存偏移量加到 cp上去*/
            FuncOffList fl = offsetArr[MAX_FUN];
            emitRM("LDA", cp, fl->offs, cp, "update current pointer");
            /* 将main函数压入调用栈中*/
            callStack.push(idK->attr.name);
            /* 读取main函数的EnterList链表*/
            int i = hash_my(idK->attr.name);
            while (strcmp(funcEnList[i]->funcName, idK->attr.name) != 0) {
                i++;
            }
            /* 为了不改变原来函数对应的EnterList链表结构，
               这里需要复制一份链表 */
            EnterList enList = copyEnList(funcEnList[i]);
            /* 为main函数新建一个tabList*/
            TokenTable tabelList[MAX_TABSIZE] = { NULL };
            tabelList[0] = nationTable;
            /* 函数只能访问自己的局部变量表和全局变量表，
               因此level置为 1*/
            int level = 1;
            cGen(comp, enList, tabelList, level);
            /* 函数出栈*/
            callStack.pop();
            emitComment("<- main");
        }
        break;
    }
    case CompK: {
        TreeNode* stmt = tree->children[1];
        bool back;
        if (enList != NULL) {
            tabList[level++] = enList->table;
            back = cGen(stmt, enList->childLink, tabList, level);
            level--;
            if (enList->father != NULL) {
                /* 若当前EnterList不是函数级别的局部符号表，
                   则将其从原来的子链表中删除*/
                enList->father->childLink = enList->brother;
            }
        }
        else {
            back = cGen(stmt, NULL, tabList, level);
        }
        return back;
    }
    case InK:
        emitComment("-> Input");
        /* 将赋值目标变量在内存中的地址置入ac中*/
        cGenMemPos(tree->children[0], tabList, level, true);
        emitRO("IN", ac1, 0, 0, "input an integer value");
        emitRM("ST_INT", ac1, 0, ac, "input: store value");
        emitComment("<- Input");
        break;
    case OutK:
        emitComment("-> Output");
        /* 将要输出变量的值置入ac中*/
        cGen(tree->children[0], NULL, tabList, level);
        emitRO("OUT", ac, 0, 0, "output an integer value");
        emitComment("<- Output");
        break;
    case ExpT:
        emitComment("-> ExpT");
        cGen(tree->children[0], NULL, tabList, level);
        emitComment("<- ExpT");
        break;
    case RetK:
        emitComment("-> RetT");
        cGen(tree->children[0], NULL, tabList, level);
        emitComment("<- RetT");
        /* return后退出本次函数调用*/
        return false;
        break;
    case IfK: {
        emitComment("-> IFK");
        TreeNode* test = tree->children[0];
        TreeNode* t1 = tree->children[1];
        TreeNode* t2 = tree->children[2];
        int savedLoc1, savedLoc2, currentLoc;
        /* 生成计算条件的表达式的代码*/
        cGen(test, NULL, tabList, level);
        savedLoc1 = emitSkip(1);
        emitComment("if: jump to else belongs here");
        /* 生成条件成立时所执行语句的代码*/
        cGen(t1, enList, tabList, level);
        /* 使enList指向下一个要访问到局部符号表*/
        enList = enList->father->childLink;
        savedLoc2 = emitSkip(1);
        emitComment("if: jump to end belongs here");
        currentLoc = emitSkip(0);
        emitBackup(savedLoc1);
        emitRM_Abs("JEQ", ac, currentLoc, "if: jmp to else");
        emitRestore();
        /* 生成条件不成立时所执行语句的代码*/
        cGen(t2, enList, tabList, level);
        currentLoc = emitSkip(0);
        emitBackup(savedLoc2);
        emitRM_Abs("LDA", pc, currentLoc, "jmp to end");
        emitRestore();
        emitComment("<- IFK");
        break;
    }
    case WhileK: {
        emitComment("-> WhileK");
        TreeNode* t1 = tree->children[0];
        TreeNode* test = tree->children[1];
        int savedLoc1 = emitSkip(0);
        emitComment("repeat: jump after body comes back here");
        /* 生成条件成立时所执行语句的代码 */
        cGen(t1, enList, tabList, level);
        /* 生成计算条件的表达式的代码 */
        cGen(test, NULL, tabList, level);
        emitRM_Abs("JNE", ac, savedLoc1, "repeat: jmp back to body");
        emitComment("<- WhileK");
        break;
    }
    case CallK: {
        emitComment("-> CallK");
        TreeNode* idK = tree->children[0];
        TreeNode* argu = tree->children[1];
        /* 读取所调用函数的EnterList链表*/
        int h = hash_my(idK->attr.name);
        while (strcmp(funcEnList[h]->funcName, idK->attr.name) != 0) {
            h++;
        }
        /* 为了不改变原来函数对应的EnterList链表结构，
           这里需要复制一份链表 */
        EnterList enList = copyEnList(funcEnList[h]);
        /* 找出当前栈顶函数的offset*/
        char* curfunName = callStack.top();
        h = hash_my(curfunName);
        FuncOffList fl = offsetArr[h];
        while (strcmp(fl->funcName, curfunName) != 0) {
            fl = fl->next;
        }
        /* 给形参进行赋值*/
        BucketList curFun, curParm;
        st_lookup(idK->attr.name, tabList, 1, curFun, true);
        TreeNode* parm = curFun->params;
        emitComment("assign arguments to parameters");
        if (argu == NULL) {
            emitComment("no parameters");
        }
        while (argu != NULL) {
            TreeNode* nextArgu = argu->sibling;
            argu->sibling = NULL;
            emitComment("-> Assign");
            if (argu->kind.exp == IdK) {
                if (cGenMemPos(argu, tabList, level, true)) {
                    /* 该实参是数组变量，进行址传递*/
                    emitComment("argu is an array, deliver the address");
                    emitRM("ST_INT", ac, tmpOffset--, mp, "op: push source array address");
                    /* 将栈顶函数的内存偏移量加到 cp上去*/
                    emitRM("LDA", cp, fl->offs, cp, "update current pointer");
                    tabList[level++] = enList->table;
                    cGenMemPos(parm, tabList, level, false);
                    emitRM("LD", ac1, ++tmpOffset, mp, "op: load source array address");
                    emitRM("ST_INT", ac1, 0, ac, "op: push address into target pos");
                }
                else {
                    /* 该实参是非数组变量，进行值传递*/
                    emitComment("argu is an variable or an item of array, deliver the value");
                    emitRM("LD", ac, 0, ac, "load variable");
                    if (argu->type == Int) {
                        emitRM("ST_INT", ac, tmpOffset--, mp, "op: push variable value");
                    }
                    else {
                        emitRM("ST_FLO", ac, tmpOffset--, mp, "op: push variable value");
                    }
                    /* 将栈顶函数的内存偏移量加到 cp上去*/
                    emitRM("LDA", cp, fl->offs, cp, "update current pointer");
                    tabList[level++] = enList->table;
                    cGenMemPos(parm, tabList, level, false);
                    emitRM("LD", ac1, ++tmpOffset, mp, "op: load variable value");
                    if (parm->type == Int) {
                        emitRM("ST_INT", ac1, 0, ac, "op: push value into target pos");
                    }
                    else {
                        emitRM("ST_FLO", ac1, 0, ac, "op: push value into target pos");
                    }
                }
            }
            else {
                /* 该实参为表达式或者常量*/
                emitComment("argu is an expression or a const, deliver the value");
                cGen(argu, NULL, tabList, level);
                if (argu->type == Int) {
                    emitRM("ST_INT", ac, tmpOffset--, mp, "op: push variable value");
                }
                else {
                    emitRM("ST_FLO", ac, tmpOffset--, mp, "op: push variable value");
                }
                /* 将栈顶函数的内存偏移量加到 cp上去*/
                emitRM("LDA", cp, fl->offs, cp, "update current pointer");
                tabList[level++] = enList->table;
                cGenMemPos(parm, tabList, level, false);
                emitRM("LD", ac1, ++tmpOffset, mp, "op: load variable value");
                if (parm->type == Int) {
                    emitRM("ST_INT", ac1, 0, ac, "op: push value into target pos");
                }
                else {
                    emitRM("ST_FLO", ac1, 0, ac, "op: push value into target pos");
                }
            }
            /* 标记为函数参数*/
            st_lookup(parm->attr.name, tabList, level, curParm, true);
            curParm->isParm = true;
            level--;
            /* current pointer回滚*/
            emitRM("LDA", cp, -fl->offs, cp, "rollback current pointer");
            emitComment("<- Assign");
            parm = parm->sibling;
            argu = nextArgu;
        }
        /* 将栈顶函数的内存偏移量加到 cp上去*/
        emitRM("LDA", cp, fl->offs, cp, "update current pointer");
        /* 把当前调用的函数名压入调用栈中*/
        callStack.push(idK->attr.name);
        /* 为调用的函数新建一个tabList*/
        TokenTable tabelList[MAX_TABSIZE] = { NULL };
        tabelList[0] = nationTable;
        /* 函数只能访问自己的局部变量表和全局变量表，
           因此level置为 1*/
        int level = 1;
        cGen(curFun->compK, enList, tabelList, level);
        /* current pointer回滚*/
        emitRM("LDA", cp, -fl->offs, cp, "rollback current pointer");
        /* 函数出栈*/
        callStack.pop();
        emitComment("<- CallK");
        break;
    }
    default:
        break;
    }
    return true;
}

/* 进行跳转的接口*/
static bool cGen(TreeNode* tree, EnterList enList, TokenTable* tabList, int level) {
    if (tree != NULL) {
        switch (tree->nodeKind) {
        case StmtK:
            if (!genStmt(tree, enList, tabList, level)) {
                /* 已执行了return语句，返回false*/
                return false;
            }
            if (enList != NULL && enList->father != NULL) {
                /* 使enList始终指向下一个要访问的局部符号表*/
                enList = enList->father->childLink;
            }
            break;
        case ExpK:
            genExp(tree, tabList, level);
            break;
        default:
            break;
        }
        cGen(tree->sibling, enList, tabList, level);
    }
    return true;
}

/* 生成中间代码的主接口*/
void codeGen(TreeNode* syntaxTree, const char* codefile) {
    char* s = (char*)malloc(strlen(codefile) + 7);
    strcpy(s, "File: ");
    strcat(s, codefile);
    emitComment("TINY Compilation to TM Code");
    emitComment(s);
    /* 生成标准开头*/
    emitComment("Standard prelude:");
    emitRM("LD", mp, 0, ac, "load maxaddress from location 0");
    emitRM("ST_INT", ac, 0, ac, "clear location 0");
    emitComment("End of standard prelude.");
    /* 生成程序的中间代码 */
    cGen(syntaxTree);
    /* 结束 */
    emitComment("End of execution.");
    emitRO("HALT", 0, 0, 0, "");
}
