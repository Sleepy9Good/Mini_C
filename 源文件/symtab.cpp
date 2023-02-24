/*************************************/
/*SYMTAB.cpp                         */
/*版本：1.0                          */
/*作者：Liji Chan                    */
/*************************************/
/*接口函数：hash_my()                   */
/*功能：生成变量名的唯一索引         */
/*************************************/
/*接口函数：st_lookup()              */
/*功能：查找变量信息                 */
/*************************************/
/*接口函数：st_insert()              */
/*功能：在符号表中插入新变量         */
/*************************************/

#include "SYMTAB.h"
#include<QString>
#include<QMessageBox>

const int SIZE = 211; /* 符号表的最大长度*/
const int SHIFT = 4; /* 哈希函数计算时每次左移的位数*/
int offset = 0; /* 变量在内存中的相对偏移量*/

/* 符号表用哈希表的方式进行查找，
 * 哈希表中每个位置保留一个链表指针，hash值相同的
 * 变量信息存放在同一个链表中。
*/
BucketList nationTable[SIZE] = { NULL }; /* 全局符号表*/

/* 用于确定变量的符号表中索引的哈希函数*/
int hash_my(const char* key) {
    int temp = 0;
    int i = 0;
    while (key[i] != '\0') {
        temp = ((temp << SHIFT) + key[i]) % SIZE;
        ++i;
    }
    return temp;
}

/* 打印出错信息*/
inline static void LogicError(int lineno, const char* message) {
    printf("Logic error at line %d: %s", lineno - 12, message);
    QString fau = "Logic error at line ";
    fau = fau + QString::number(lineno -12) + ": " + QString(message);
    QMessageBox::about(NULL,"错误",fau);return;
}

/* 依次从当前符号表往上遍历到全局符号表，
 * 查看是否存在该变量名，
 * 存在则返回其memloc，否则返回-1
 */
int st_lookup(const char* name, TokenTable tableList[], int tableNum, BucketList& l, bool local) {
    int h = hash_my(name);
    TokenTable curtab;
    while (tableNum > 0) {
        tableNum--;
        curtab = tableList[tableNum];
        l = curtab[h];
        while (l != NULL && strcmp(l->name, name) != 0) {
            l = l->next;
        }
        if (l != NULL) {
            /* 因为函数的memloc = -1，所以这里需要返回一个正数*/
            return (l->memloc == -1) ? 1 : l->memloc;
        }
        if (local) {
            /* local为true说明只在当前符号表上进行查找*/
            return -1;
        }
    }
    return -1;
}

/* 插入操作的子函数*/
void st_insert(TreeNode* t, int loc, TokenTable hashTable, bool isLocal) {
    TreeNode* idK = (t->children[1] != NULL) ? t->children[0] : t;
    if (loc == -1) { /* loc = -1 说明符号表中不存在该变量名*/
        BucketList tuple = (BucketList)malloc(sizeof(BucketListRec));
        if (tuple != NULL) {
            tuple->type = idK->type;
            tuple->name = idK->attr.name; /* 注意这里直接指向原数组的内存*/
            tuple->memloc = -1;
            tuple->params = NULL;
            tuple->compK = NULL;
            tuple->isParm = false;
            tuple->isFunc = false;
            tuple->isLocal = isLocal;
            int h = hash_my(idK->attr.name);
            tuple->next = hashTable[h];
            hashTable[h] = tuple;
        }
        switch (idK->type)
        {
        case IntArr:
        case FloArr: {
            TreeNode* len = idK->children[0];
            if (len != NULL) {
                if (len->kind.exp != IntK) {
                    LogicError(t->lineno, "使用了非整数常量进行数组长度初始化\n");
                }
                else if (len->attr._int <= 0) {
                    LogicError(t->lineno, "使用了非正整数进行数组长度初始化\n");
                }
                else {
                    tuple->memloc = offset;
                    offset += len->attr._int;
                }
            }
            else {
                tuple->memloc = offset++;
            }
            break;
        }
        case Int:
        case Float:
        case Void: {
            TreeNode* param = t->children[1];
            if (param != NULL && param->kind.stmt == ParmK) {
                /* 条件成立则为函数，否则为基本类型变量*/
                tuple->params = param->children[0];
                tuple->compK = t->children[2];
                tuple->isFunc = true;
            }
            else {
                tuple->memloc = offset++;
            }
            break;
        }
        default:
            break;
        }
    }
    else {
        LogicError(t->lineno, "重复定义了同一变量名\n");
    }
}
