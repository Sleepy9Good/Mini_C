#pragma once
#ifndef _TM_H_
#define _TM_H_
#define FALSE 0

#include<tmwid.h>
#include <QWaitCondition>
#include <QMutex>
#include <QThread>
#include<QObject>

static bool stefla=false;
static int done;
static FILE* pgm;

extern int iloc;
extern int dloc;
extern int traceflag;
extern int icountflag;
extern int isFloat;

typedef struct {
    union {
        int _int;
        float _flo;
    } val;
    int isFlo;  /* 标记是否存放浮点数*/
} UNIT;

typedef struct {
    int iop;
    int iarg1;
    UNIT iarg2;
    int iarg3;
} INSTRUCTION;

extern INSTRUCTION iMem[];
extern UNIT dMem[];
extern UNIT reg[];

int doCommand(void);




/* 解释器主接口*/
void simulate(const char* pgmName);

#endif // !_TM_H_
