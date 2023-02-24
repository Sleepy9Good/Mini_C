/*************************************/
/*ANALYSE.cpp                        */
/*版本：1.0                          */
/*作者：Liji Chan                    */
/*************************************/
/*接口函数：buildSymtab()            */
/*功能：生成所有符号表，             */
/*      并进行类型检查               */
/*************************************/

#include "ANALYSE.h"
#include "SYMTAB.h"
#include<QString>
#include<QMessageBox>

using namespace std;

const int MAX_FUN = 211; /* 最多允许源文件中定义的函数数目*/
static const int MAX_TABSIZE = 14; /* 最多同时存在的符号表层数*/
int level = 0; /* 当前的作用域级别*/
TokenTable tabList[MAX_TABSIZE] = { NULL }; /* 按作用域级别排序的符号表数组*/

/* 该哈希表为每个函数保存一个链表,
 * 每个链表按作用域级别顺序保存该函数在
 * 类型检查中曾经建立过的符号表入口，
 * 以便生成中间代码时可以快速访问到对应的符号表
 */
EnterList funcEnList[MAX_FUN + 50] = { NULL };

/* 存放全局变量的最大内存单元和各个函数的最大内存偏移量*/
FuncOffList offsetArr[MAX_FUN + 1];

/* 记录当前所检查到的函数的名字*/
char* curFuncName = NULL;

/* 记录当前所检查到的函数的返回值类型*/
ExpType returnType = Void;

/* 判断函数体中是否出现过return语句*/
static bool hasReturn;

/* 函数提前声明*/
static void typeCheck(TreeNode* t, EnterList enList);
static void checkNode(TreeNode* t, TokenTable localTab = NULL, EnterList enList = NULL, bool isArgu = false);

/* 打印出错信息*/
inline static void TypeError(int lineno, const char* message) {
//    printf("Type error at line %d: %s", lineno - 8, message);
    QString fau = "Syntax error at line ";
    fau = fau + QString::number(lineno - 12) + ": " + QString(message);
    QMessageBox::about(NULL,"错误",fau);return;
}


/* 检查表达式类型结点*/
static void checkExp(TreeNode* t, bool isArgu) {
    switch (t->kind.exp) {
    case OpK: {
        TreeNode* c1 = t->children[0];
        TreeNode* c2 = t->children[1];
        checkNode(c1);
        checkNode(c2);
        switch (t->attr.op) {
            /* 进行非赋值运算时，不允许运算符左边是尚未初始化的变量名*/
        case LT:    case LT_EQ: case RT:
        case RT_EQ: case EQ:    case UNEQ:
            if (c1->type == Void || c2->type == Void) {
                TypeError(t->lineno, "操作数不可以是空类型\n");
            }
            else {
                t->type = Int;
            }
            break;
        case PLUS: case MINUS: case MULTI: case DIVI:
            if (c1->type == Void || c2->type == Void) {
                TypeError(t->lineno, "操作数不可以是空类型\n");
            }
            else if (c1->type == Float || c2->type == Float) {
                t->type = Float;
            }
            else {
                t->type = Int;
            }
            break;
        case MOD:
            if (c1->type == Void || c2->type == Void) {
                TypeError(t->lineno, "操作数不可以是空类型\n");
            }
            else if (c1->type == Float || c2->type == Float) {
                TypeError(t->lineno, "浮点数不支持 % 运算\n");

            }
            else {
                t->type = Int;
            }
            break;
        case ASSIGN:
            /* 赋值表达式的数据类型与被赋值变量的数据类型一致*/
            t->type = c1->type;
            break;
        default:
            break;
        }
        break;
    }
    case IntK:
        t->type = Int;
        break;
    case FloatK:
        t->type = Float;
        break;
    case IdK:
        BucketList l;
        if (st_lookup(t->attr.name, tabList, level, l, false) == -1 || l->isFunc) {
            TypeError(t->lineno, "使用了未定义的变量名\n");
        }
        else if (l->type == IntArr || l->type == FloArr) {
            /* 数组类型变量*/
            if (isArgu) {
                /* 数组作为实参传递时可以不加索引*/
                TreeNode* index = t->children[0];
                checkNode(index);
                if (index != NULL) {
                    switch (index->type) {
                    case Void:
                        TypeError(t->lineno, "使用了空类型的索引\n");
                        break;
                    case Float:
                        TypeError(t->lineno, "使用了浮点数类型的索引\n");
                        break;
                    case Int:
                        t->type = (l->type == IntArr) ? Int : Float;
                        break;
                    default:
                        break;
                    }
                }
                else {
                    t->type = l->type;
                }
            }
            else {
                /* 数组不作为实参传递时必须加索引*/
                TreeNode* index = t->children[0];
                checkNode(index);
                if (index == NULL) {
                    TypeError(t->lineno, "使用数组变量时未提供索引\n");
                }
                else {
                    switch (index->type) {
                    case Void:
                        TypeError(t->lineno, "使用了空类型的索引\n");
                        break;
                    case Float:
                        TypeError(t->lineno, "使用了浮点数类型的索引\n");
                        break;
                    case Int:
                        t->type = (l->type == IntArr) ? Int : Float;
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        else {
            /* 基本类型变量*/
            t->type = l->type;
        }
        break;
    default:
        break;
    }
}

/* 检查语句类型结点*/
static void checkStmt(TreeNode* t, TokenTable localTab, EnterList enList) {
    switch (t->kind.stmt) {
    case RetK:
        /* 更新标志*/
        hasReturn = true;
    case ExpT:
        checkNode(t->children[0]); /* 表达式结点*/
        break;
    case IfK:
        checkNode(t->children[0]); /* 跳转条件*/
        checkNode(t->children[1], NULL, enList); /* True语句*/
        checkNode(t->children[2], NULL, enList); /* False语句*/
        if (t->children[0]->type == Void) {
            TypeError(t->lineno, "判断条件不能为空类型\n");
        }
        break;
    case WhileK:
        checkNode(t->children[0], NULL, enList); /* True语句*/
        checkNode(t->children[1]); /* 循环条件*/
        if (t->children[1]->type == Void) {
            TypeError(t->lineno, "判断条件不能为空类型\n");
        }
        break;
    case CompK: {
        if (localTab == NULL) {
            localTab = new BucketList[SIZE]{ NULL };
            /* 建立新Enter表项*/
            EnterList newEn = (EnterList)malloc(sizeof(Enter));
            if (newEn != NULL) {
                newEn->funcName = NULL;
                newEn->table = localTab;
                newEn->father = enList;
                newEn->brother = NULL;
                newEn->childLink = NULL;
            }
            /* 将新表项插入到childLink链表的末尾*/
            if (enList->childLink != NULL) {
                enList = enList->childLink;
                while (enList->brother != NULL) {
                    enList = enList->brother;
                }
                enList->brother = newEn;
                enList = newEn;
            }
            else {
                enList->childLink = newEn;
                enList = newEn;
            }
        }
        TreeNode* def = t->children[0]; /* 局部定义*/
        TreeNode* stmt = t->children[1]; /* 语句组*/
        buildSymtab(def, localTab);
        typeCheck(stmt, enList);
        level--;
        /* 检查函数内各返回值语句是否合法*/
        while (stmt != NULL) {
            if (stmt->kind.stmt == RetK) {
                switch (returnType) {
                case Void:
                    if (stmt->children[0] != NULL && stmt->children[0]->type != Void) {
                        TypeError(stmt->lineno, "此处返回值类型应为 void\n");
                    }
                    break;
                case Int:
                    if (stmt->children[0] == NULL) {
                        TypeError(stmt->lineno, "此处缺少 int型返回值\n");
                    }
                    else if (stmt->children[0]->type == Float){
                        TypeError(stmt->lineno, "此处返回值类型应为 int\n");
                    }
                    break;
                case Float:
                    if (stmt->children[0] == NULL) {
                        TypeError(stmt->lineno, "此处缺少 float型返回值\n");
                    }
                    else if (stmt->children[0]->type == Int) {
                        TypeError(stmt->lineno, "此处返回值类型应为 float\n");
                    }
                    break;
                default:
                    break;
                }
            }
            stmt = stmt->sibling;
        }
        break;
    }
    case CallK: {
        TreeNode* idK = t->children[0];
        TreeNode* argu = t->children[1];
        BucketList l;
        if (st_lookup(idK->attr.name, tabList, 1, l, true) == -1 || !l->isFunc) {
            TypeError(t->lineno, "该函数名不存在\n");
        }
        else {
            TreeNode* p = l->params;
            checkNode(argu, NULL, NULL, true);
            while (argu != NULL && p != NULL && argu->type == p->type) {
                argu = argu->sibling;
                checkNode(argu, NULL, NULL, true);
                p = p->sibling;
            }
            if (argu != NULL || p != NULL) {
                TypeError(t->lineno, "函数调用提供的参数列表不匹配\n");
            }
            else {
                t->type = l->type;
            }
        }
        break;
    }
    default:
        break;
    }
}

/* 类型检查*/
static void checkNode(TreeNode* t, TokenTable localTab, EnterList enList, bool isArgu) {
    if (t == NULL) {
        return;
    }
    switch (t->nodeKind) {
    case ExpK:
        checkExp(t, isArgu);
        break;
    case StmtK:
        checkStmt(t, localTab, enList);
        break;
    default:
        break;
    }
}

/* 类型检查接口*/
static void typeCheck(TreeNode* t, EnterList enList) {
    while (t != NULL) {
        checkNode(t, NULL, enList);
        t = t->sibling;
    }
}

/* 向某个符号表中加入变量*/
static void insertNode(TreeNode* t, TokenTable hashTable) {
    BucketList l; /* 此处定义只是为了顺利调用查询函数*/
    /* 变量类型*/
    if (t->kind.stmt == DefId) {
        /* 当level = 1时，说明该变量是全局变量*/
        bool isLocal = (level != 1) ? true : false;
        if (!isLocal && curFuncName != NULL) {
            /* 该变量是全局变量，且上一个定义是函数*/
            /* 保存该函数的偏移量*/
            FuncOffList fl;
            fl = (FuncOffList)malloc(sizeof(FuncOffset));
            fl->funcName = curFuncName;
            fl->offs = offset;
            int h = hash_my(curFuncName);
            fl->next = offsetArr[h];
            offsetArr[h] = fl;
            /* 置为空，以记录本次定义为全局变量*/
            curFuncName = NULL;
            /* 取出全局变量的offset，全局变量区继续扩展*/
            fl = offsetArr[MAX_FUN];
            offset = fl->offs;
        }
        /* 将所定义的变量信息加入到当前符号表中*/
        st_insert(t->children[0], st_lookup(t->children[0]->attr.name, tabList, level, l, true), hashTable, isLocal);
    }
    /* 函数类型*/
    else if (t->kind.stmt == DefFun) {
        TreeNode* idK = t->children[0];
        TreeNode* param = t->children[1];
        TreeNode* comp = t->children[2];
        /* 将所定义的函数信息加入到当前符号表（全局符号表）中*/
        st_insert(t, st_lookup(idK->attr.name, tabList, 1, l, true), hashTable, true);
        /* 每次建立新函数的局部符号表之前保存当前偏移量*/
        FuncOffList fl;
        if (curFuncName == NULL) {
            /* 说明上一个全局定义是变量，
               保存全局偏移量*/
            fl = (offsetArr[MAX_FUN] == NULL) ? (FuncOffList)malloc(sizeof(FuncOffset)) : offsetArr[MAX_FUN];
            fl->offs = offset;
            offsetArr[MAX_FUN] = fl;
        }
        else {
            /* 说明上一个全局定义是函数，
               将上一个函数的offset保存*/
            fl = (FuncOffList)malloc(sizeof(FuncOffset));
            if (fl != NULL) {
                fl->next = NULL;
                fl->offs = offset;
                fl->funcName = curFuncName;
                int h = hash_my(curFuncName);
                fl->next = offsetArr[h];
                offsetArr[h] = fl;
            }
        }
        /* 重置偏移量*/
        offset = 0;
        /* 为当前函数建立Enter链表*/
        TokenTable localTab = new BucketList[SIZE]{ NULL };
        EnterList enList = (EnterList)malloc(sizeof(Enter));
        if (enList != NULL) {
            enList->funcName = idK->attr.name; /* 数组浅拷贝*/
            enList->table = localTab;
            enList->father = NULL;
            enList->brother = NULL;
            enList->childLink = NULL;
        }
        /* 采用线性哈希表的方式存储hash值相同函数的Enter链表*/
        int i = hash_my(idK->attr.name);
        while (funcEnList[i] != NULL) {
            i++;
        }
        funcEnList[i] = enList;
        /* 保存新函数名*/
        curFuncName = idK->attr.name;
        /* 更新函数返回值类型*/
        returnType = idK->type;
        /* 重置标志*/
        hasReturn = false;
        /* 先把函数参数添加到该函数的局部符号表中*/
        tabList[level++] = localTab;
        if (param != NULL) {
            param = param->children[0];
            while (param != NULL) {
                st_insert(param, st_lookup(param->attr.name, tabList, level, l, true), localTab, true);
                param = param->sibling;
            }
        }
        level--;
        /* 对函数体进行类型检查*/
        checkNode(comp, localTab, enList);
        if (idK->type != Void && !hasReturn) {
            TypeError(comp->lineno, "函数体缺少return语句\n");
        }
        /* 由于main函数是最后定义的一个函数，
           其offset需要另外保存*/
        if (strcmp(idK->attr.name, "main") == 0) {
            FuncOffList fl = (FuncOffList)malloc(sizeof(FuncOffset));
            if (fl != NULL) {
                fl->offs = offset;
                fl->funcName = curFuncName;
                int h = hash_my(curFuncName);
                fl->next = offsetArr[h];
                offsetArr[h] = fl;
            }
        }
    }
    else {
        TypeError(t->lineno, "遇到了非定义语句类型\n");
    }
}

/* 生成符号表*/
void buildSymtab(TreeNode* syntaxTree, TokenTable hashTable) {
    tabList[level++] = hashTable;
    while (syntaxTree != NULL) {
        insertNode(syntaxTree, hashTable);
        syntaxTree = syntaxTree->sibling;
    }
}
