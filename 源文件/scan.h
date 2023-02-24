#pragma once

#ifndef _SCAN_H_
#define _SCAN_H_
#include "GLOBAL.h"
using namespace std;
/* 有穷状态机的六个状态*/
typedef enum {
    START, INCOMMENT, INNUM, INID, INOP, DONE
} StateType;

extern int linepos; /* 缓存数组当前下标*/
extern int bufsize; /* 缓存数组当前大小*/
extern bool EOF_flag; /* 标志是否读取到文件末尾*/

/* 获取下一个token，返回其类型*/
TokenType getToken();
#endif
