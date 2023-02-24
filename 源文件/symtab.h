#pragma once
#ifndef _SYMTAB_H
#define _SYMTAB_H
#include "GLOBAL.h"
#include <string.h>
#include <malloc.h>

/* 用于确定变量的符号表中索引的哈希函数*/
int hash_my(const char* name);

/* 依次从当前符号表往上遍历到全局符号表，
 * 查看是否存在该变量名，
 * 存在则返回其memloc，否则返回-1
 */
int st_lookup(const char* name, TokenTable tableList[], int tableNum, BucketList& l, bool local);

/* 插入操作的子函数*/
void st_insert(TreeNode* t, int loc, TokenTable hashTable, bool isLocal);

extern BucketList nationTable[];

extern const int SIZE;

extern int offset;
#endif
