/*************************************/
/*TM.cpp                             */
/*版本：1.0                          */
/*作者：Liji Chan                    */
/*************************************/
/*接口函数：simulate()               */
/*功能：解释中间代码并执行           */
/*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "CODE.h"
#include<tmwid.h>
#include<QString>
#include<QMessageBox>
#include <QCoreApplication>
#include <QInputDialog>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/******* const *******/
#define   IADDR_SIZE  4096 /* increase for large programs */
#define   DADDR_SIZE  1024 /* increase for large programs */
#define   NO_REGS 8
#define   PC_REG  7

#define   LINESIZE  121
#define   WORDSIZE  20

/******* type  *******/

typedef enum {
    opclRR,     /* reg operands r,s,t */
    opclRM,     /* reg r, mem d+s */
    opclRA      /* reg r, int d+s */
} OPCLASS;

typedef enum {
    /* RR instructions */
    opHALT,    /* RR     halt, operands are ignored */
    opIN,      /* RR     read into reg(r); s and t are ignored */
    opOUT,     /* RR     write from reg(r), s and t are ignored */
    opADD,    /* RR     reg(r) = reg(s)+reg(t) */
    opSUB,    /* RR     reg(r) = reg(s)-reg(t) */
    opMUL,    /* RR     reg(r) = reg(s)*reg(t) */
    opDIV,    /* RR     reg(r) = reg(s)/reg(t) */
    opMOD,    /* RR     reg(r) = reg(s)%reg(t) */
    opRRLim,   /* limit of RR opcodes */

    /* RM instructions */
    opLD,          /* RM     reg(r) = mem(d+reg(s)) */
    opST_INT,      /* RM     mem(d+reg(s)) = reg(r) */
    opST_FLO,      /* RM     mem(d+reg(s)) = reg(r) */
    opRMLim,       /* Limit of RM opcodes */

    /* RA instructions */
    opLDA,     /* RA     reg(r) = d+reg(s) */
    opLDC,     /* RA     reg(r) = d ; reg(s) is ignored */
    opJLT,     /* RA     if reg(r)<0 then reg(7) = d+reg(s) */
    opJLE,     /* RA     if reg(r)<=0 then reg(7) = d+reg(s) */
    opJGT,     /* RA     if reg(r)>0 then reg(7) = d+reg(s) */
    opJGE,     /* RA     if reg(r)>=0 then reg(7) = d+reg(s) */
    opJEQ,     /* RA     if reg(r)==0 then reg(7) = d+reg(s) */
    opJNE,     /* RA     if reg(r)!=0 then reg(7) = d+reg(s) */
    opRALim    /* Limit of RA opcodes */
} OPCODE;

typedef enum {
    srOKAY,
    srHALT,
    srIMEM_ERR,
    srDMEM_ERR,
    srZERODIVIDE
} STEPRESULT;

/* 声明数据单元*/
//typedef struct {
//    union {
//        int _int;
//        float _flo;
//    } val;
//    int isFlo;  /* 标记是否存放浮点数*/
//} UNIT;

//typedef struct {
//    int iop;
//    int iarg1;
//    UNIT iarg2;
//    int iarg3;
//} INSTRUCTION;

/******** vars ********/
int iloc = 0;
int dloc = 0;
int traceflag = FALSE;
int icountflag = FALSE;
int isFloat = FALSE;
QString tstr;
extern void simshow(QString arr);
extern void input_my(char*);

extern QMutex pause_m;
QString tqstr;//临时
tmwid* tmform;

INSTRUCTION iMem[IADDR_SIZE];
UNIT dMem[DADDR_SIZE];
UNIT reg[NO_REGS];

static const char* opCodeTab[]
= { "HALT","IN","OUT","ADD","SUB","MUL","DIV","MOD","????",
/* RR opcodes */
"LD","ST_INT","ST_FLO","????", /* RM opcodes */
"LDA","LDC","JLT","JLE","JGT","JGE","JEQ","JNE","????"
/* RA opcodes */
};

static const char* stepResultTab[]
= { "OK","Halted","Instruction Memory Fault",
   "Data Memory Fault","Division by 0"
};

//static char pgmName[20];

static char in_Line[LINESIZE];
static int lineLen;
static int inCol;
static float num;
static char word[WORDSIZE];
static char ch;

/********************************************/
static int opClass(int c)
{
    if (c <= opRRLim) return (opclRR);
    else if (c <= opRMLim) return (opclRM);
    else                    return (opclRA);
} /* opClass */

/********************************************/
static void writeInstruction(int loc)
{
    printf("%5d: ", loc);
    tqstr=QString("%1: ").arg(loc,5,10,QLatin1Char(' '));
        simshow(tqstr);
    if ((loc >= 0) && (loc < IADDR_SIZE))
    {
        printf("%6s%3d,", opCodeTab[iMem[loc].iop], iMem[loc].iarg1);
        tqstr=QString("%1%2,").arg(opCodeTab[iMem[loc].iop],6,QLatin1Char(' ')).
                        arg(iMem[loc].iarg1,3,10,QLatin1Char(' '));
                simshow(tqstr);

        switch (opClass(iMem[loc].iop))
        {
        case opclRR: printf("%1d,%1d", iMem[loc].iarg2.val._int, iMem[loc].iarg3);
            tqstr=QString("%1,%2").arg(iMem[loc].iarg2.val._int,1,10,QLatin1Char(' ')).
                                arg(iMem[loc].iarg3,1,10,QLatin1Char(' '));
                        simshow(tqstr);
            break;
        case opclRM:
        case opclRA:
            if (iMem[loc].iarg2.isFlo) {
                printf("%3d(%1d)", iMem[loc].iarg2.val._int, iMem[loc].iarg3);
                tqstr=QString("%1(%2)").arg(iMem[loc].iarg2.val._int,3,10,QLatin1Char(' ')).
                                        arg(iMem[loc].iarg3,1,10,QLatin1Char(' '));
                                simshow(tqstr);
            }
            else {
                printf("%f(%1d)", iMem[loc].iarg2.val._flo, iMem[loc].iarg3);
                tqstr=QString("%1(%2)").arg(iMem[loc].iarg2.val._flo).
                                        arg(iMem[loc].iarg3,1,10,QLatin1Char(' '));
                                simshow(tqstr);
            }
            break;
        }
        printf("\n");
        simshow("\n");
    }
} /* writeInstruction */

/********************************************/
static void getCh(void)
{
    if (++inCol < lineLen)
        ch = in_Line[inCol];
    else ch = ' ';
} /* getCh */

/********************************************/
static int nonBlank(void)
{
    while ((inCol < lineLen)
        && (in_Line[inCol] == ' '))
        inCol++;
    if (inCol < lineLen)
    {
        ch = in_Line[inCol];
        return TRUE;
    }
    else
    {
        ch = ' ';
        return FALSE;
    }
} /* nonBlank */

/********************************************/
static int getNum(void)
{
    int sign;
    int inte;
    float deci;
    int temp = FALSE;
    num = 0;
    do
    {
        sign = 1;
        while (nonBlank() && ((ch == '+') || (ch == '-')))
        {
            temp = FALSE;
            if (ch == '-')  sign = -sign;
            getCh();
        }
        inte = 0;
        nonBlank();
        while (isdigit(ch))
        {
            temp = TRUE;
            inte = inte * 10 + (ch - '0');
            getCh();
        }
        deci = inte;
        isFloat = FALSE;
        if (ch == '.') /* 识别浮点数的小数部分*/
        {
            isFloat = TRUE;
            getCh();
            int expo = 0;
            while (isdigit(ch)) {
                deci += (ch - '0') / (float)pow(10, ++expo);
                getCh();
            }
        }
        num = num + (deci * sign);
    } while ((nonBlank()) && ((ch == '+') || (ch == '-')));
    return temp;
} /* getNum */

/********************************************/
static int getWord(void)
{
    int temp = FALSE;
    int length = 0;
    if (nonBlank())
    {
        while (isalnum(ch) || ch == '_')
        {
            if (length < WORDSIZE - 1) word[length++] = ch;
            getCh();
        }
        word[length] = '\0';
        temp = (length != 0);
    }
    return temp;
} /* getWord */

/********************************************/
static int skipCh(char c)
{
    int temp = FALSE;
    if (nonBlank() && (ch == c))
    {
        getCh();
        temp = TRUE;
    }
    return temp;
} /* skipCh */

/********************************************/
static int atEOL(void)
{
    return (!nonBlank());
} /* atEOL */

/********************************************/
static int error(const char* msg, int lineNo, int instNo)
{
    printf("Line %d", lineNo);
    tqstr=QString("Line %1").arg(lineNo);
        simshow(tqstr);
        if (instNo >= 0){ printf(" (Instruction %d)", instNo);
                tqstr=QString(" (Instruction %1)").arg(instNo);
                simshow(tqstr);}
            printf("   %s\n", msg);
            tqstr=QString("   %1\n").arg(msg);
            simshow(tqstr);
    return FALSE;
} /* error */

/********************************************/
static int readInstructions(void)
{
    OPCODE op;
    int arg1, arg3;
    UNIT arg2;
    int loc, regNo, lineNo;
    /* 初始化寄存器堆*/
    for (loc = 0; loc < NO_REGS; loc++) {
        reg[loc].isFlo = FALSE;
        reg[loc].val._int = 0;
    }
    /* 初始化内存*/
    for (loc = 0; loc < DADDR_SIZE; loc++) {
        dMem[loc].isFlo = FALSE;
        dMem[loc].val._int = 0;
    }
    /* 第一个单元存放内存的最大下标*/
    dMem[0].val._int = DADDR_SIZE - 1;
    /* 初始化指令寄存器*/
    for (loc = 0; loc < IADDR_SIZE; loc++)
    {
        iMem[loc].iop = opHALT;
        iMem[loc].iarg1 = 0;
        iMem[loc].iarg2.isFlo = FALSE;
        iMem[loc].iarg2.val._int = 0;
        iMem[loc].iarg3 = 0;
    }
    lineNo = 0;
    while (!feof(pgm))
    {
        fgets(in_Line, LINESIZE - 2, pgm);
        inCol = 0;
        lineNo++;
        lineLen = strlen(in_Line) - 1;
        if (in_Line[lineLen] == '\n') in_Line[lineLen] = '\0';
        else in_Line[++lineLen] = '\0';
        if ((nonBlank()) && (in_Line[inCol] != '*'))
        {
            if (!getNum())
                return error("Bad location", lineNo, -1);
            loc = num;
            if (loc > IADDR_SIZE)
                return error("Location too large", lineNo, loc);
            if (!skipCh(':'))
                return error("Missing colon", lineNo, loc);
            if (!getWord())
                return error("Missing opcode", lineNo, loc);
            op = opHALT;
            while ((op < opRALim)
                && (strncmp(opCodeTab[op], word, 4) != 0))
                op = OPCODE(op + 1);
            if (strncmp(opCodeTab[op], word, 4) != 0)
                return error("Illegal opcode", lineNo, loc);
            switch (opClass(op))
            {
            case opclRR:
                /***********************************/
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return error("Bad first register", lineNo, loc);
                arg1 = num;
                if (!skipCh(','))
                    return error("Missing comma", lineNo, loc);
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return error("Bad second register", lineNo, loc);
                arg2.isFlo = FALSE;
                arg2.val._int = (int)num;
                if (!skipCh(','))
                    return error("Missing comma", lineNo, loc);
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return error("Bad third register", lineNo, loc);
                arg3 = num;
                break;

            case opclRM:
            case opclRA:
                /***********************************/
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return error("Bad first register", lineNo, loc);
                arg1 = num;
                if (!skipCh(','))
                    return error("Missing comma", lineNo, loc);
                if (!getNum())
                    return error("Bad displacement", lineNo, loc);
                arg2.isFlo = isFloat;
                if (isFloat) {
                    arg2.val._flo = num;
                }
                else {
                    arg2.val._int = (int)num;
                }
                if (!skipCh('(') && !skipCh(','))
                    return error("Missing LParen", lineNo, loc);
                if ((!getNum()) || (num < 0) || (num >= NO_REGS))
                    return error("Bad second register", lineNo, loc);
                arg3 = num;
                break;
            }
            iMem[loc].iop = op;
            iMem[loc].iarg1 = arg1;
            iMem[loc].iarg2 = arg2;
            iMem[loc].iarg3 = arg3;
        }
    }
    return TRUE;
} /* readInstructions */


/********************************************/
static STEPRESULT stepTM(void)
{
    INSTRUCTION currentinstruction;
    int pct;
    int r, s, t, m;
    int ok;
    int arg1, arg3;
    UNIT arg2;
    pct = reg[PC_REG].val._int;
    if ((pct < 0) || (pct > IADDR_SIZE))
        return srIMEM_ERR;
    reg[PC_REG].val._int = pct + 1;
    currentinstruction = iMem[pct];
    arg1 = currentinstruction.iarg1;
    arg2 = currentinstruction.iarg2;
    arg3 = currentinstruction.iarg3;
    switch (opClass(currentinstruction.iop))
    {
    case opclRR:
        /***********************************/
        r = arg1;
        s = arg2.val._int;
        t = arg3;
        break;

    case opclRM:
        /***********************************/
        r = arg1;
        s = arg3;
        m = ((arg2.isFlo) ? arg2.val._flo : arg2.val._int) +
            ((reg[s].isFlo) ? reg[s].val._flo : reg[s].val._int);
        if ((m < 0) || (m > DADDR_SIZE))
            return srDMEM_ERR;
        break;

    case opclRA:
        /***********************************/
        r = arg1;
        s = arg3;
        m = ((arg2.isFlo) ? arg2.val._flo : arg2.val._int) +
            ((reg[s].isFlo) ? reg[s].val._flo : reg[s].val._int);
        break;
    } /* case */

    switch (currentinstruction.iop)
    { /* RR instructions */
    case opHALT:
        /***********************************/
        printf("HALT: %1d,%1d,%1d\n", r, s, t);
        tqstr=QString("HALT: %1,%2,%3\n").arg(r,1,10,QLatin1Char(' ')).
                        arg(s,1,10,QLatin1Char(' ')).arg(t,1,10,QLatin1Char(' '));
                simshow(tqstr);
        return srHALT;
        /* break; */

    case opIN:
        /***********************************/
        do
        {
            printf("Enter value for IN instruction: ");
            simshow("Enter value for IN instruction: ");
            fflush(stdin);
            fflush(stdout);
//            gets_s(in_Line);
            QInputDialog inpdia;inpdia.show();
                        QString arr = inpdia.getText(tmform, "数值获取","请输入数值", QLineEdit::Normal);
                        char* chrs = arr.toLatin1().data();
                        strcpy(in_Line,chrs);
                        simshow("\ninput: ");
                        simshow(arr);simshow("\n");
            lineLen = strlen(in_Line);
            inCol = 0;
            ok = getNum();
            if (!ok)  {printf("Illegal value\n");simshow("Illegal value\n");}
            else {
                if (isFloat) {
                    printf("Only accept an integer! Again please.\n");
                    simshow("Only accept an integer! Again please.\n");
                    continue;
                }
                reg[r].val._int = (int)num;
            }
        } while (!ok);
        break;

    case opOUT:
        printf("OUT instruction prints: %d\n", reg[r].val._int);
        tqstr=QString("OUT instruction prints: %1\n").arg(reg[r].val._int);
                simshow(tqstr);
        break;

    case opADD: {
        float op1 = (reg[s].isFlo) ? reg[s].val._flo : (float)reg[s].val._int;
        float op2 = (reg[t].isFlo) ? reg[t].val._flo : (float)reg[t].val._int;
        reg[r].isFlo = (reg[s].isFlo + reg[t].isFlo) ? TRUE : FALSE;
        if (reg[r].isFlo) {
            reg[r].val._flo = op1 + op2;
        }
        else {
            reg[r].val._int = reg[s].val._int + reg[t].val._int;
        }
        break;
    }

    case opSUB: {
        float op1 = (reg[s].isFlo) ? reg[s].val._flo : (float)reg[s].val._int;
        float op2 = (reg[t].isFlo) ? reg[t].val._flo : (float)reg[t].val._int;
        reg[r].isFlo = (reg[s].isFlo + reg[t].isFlo) ? TRUE : FALSE;
        if (reg[r].isFlo) {
            reg[r].val._flo = op1 - op2;
        }
        else {
            reg[r].val._int = reg[s].val._int - reg[t].val._int;
        }
        break;
    }

    case opMUL: {
        float op1 = (reg[s].isFlo) ? reg[s].val._flo : (float)reg[s].val._int;
        float op2 = (reg[t].isFlo) ? reg[t].val._flo : (float)reg[t].val._int;
        reg[r].isFlo = (reg[s].isFlo + reg[t].isFlo) ? TRUE : FALSE;
        if (reg[r].isFlo) {
            reg[r].val._flo = op1 * op2;
        }
        else {
            reg[r].val._int = reg[s].val._int * reg[t].val._int;
        }
        break;
    }

    case opDIV: {
        /***********************************/
        float op1 = (reg[s].isFlo) ? reg[s].val._flo : (float)reg[s].val._int;
        float op2 = (reg[t].isFlo) ? reg[t].val._flo : (float)reg[t].val._int;
        reg[r].isFlo = (reg[s].isFlo + reg[t].isFlo) ? TRUE : FALSE;
        if (reg[t].isFlo) {
            /* 检查除0异常*/
            if (op2 != 0) reg[r].val._flo = op1 / op2;
            else return srZERODIVIDE;
        }
        else {
            if (reg[t].val._int != 0) reg[r].val._int = reg[s].val._int / reg[t].val._int;
            else return srZERODIVIDE;
        }
        break;
    }

    case opMOD:
        /***********************************/
        reg[r].isFlo = FALSE;
        /* 检查除0异常*/
        if (reg[t].val._int != 0) reg[r].val._int = reg[s].val._int % reg[t].val._int;
        else return srZERODIVIDE;
        break;

        /*************** RM instructions ********************/
    case opLD:    reg[r] = dMem[m];  break;
    case opST_INT:
        dMem[m].isFlo = FALSE;
        if (reg[r].isFlo) {
            dMem[m].val._int = (int)reg[r].val._flo;
        }
        else {
            dMem[m].val._int = reg[r].val._int;
        }
        break;
    case opST_FLO:
        dMem[m].isFlo = TRUE;
        if (reg[r].isFlo) {
            dMem[m].val._flo = reg[r].val._flo;
        }
        else {
            dMem[m].val._flo = (float)reg[r].val._int;
        }
        break;

        /*************** RA instructions ********************/
    case opLDA:
        reg[r].isFlo = FALSE;
        reg[r].val._int = m;
        break;
    case opLDC:    reg[r] = currentinstruction.iarg2;   break;
    case opJLT:
        if (reg[r].isFlo && reg[r].val._flo < 0) {
            reg[PC_REG].val._int = m;
        }
        else if (!reg[r].isFlo && reg[r].val._int < 0){
            reg[PC_REG].val._int = m;
        }
        break;
    case opJLE:
        if (reg[r].isFlo && reg[r].val._flo <= 0) {
            reg[PC_REG].val._int = m;
        }
        else if (!reg[r].isFlo && reg[r].val._int <= 0) {
            reg[PC_REG].val._int = m;
        }
        break;
    case opJGT:
        if (reg[r].isFlo && reg[r].val._flo > 0) {
            reg[PC_REG].val._int = m;
        }
        else if (!reg[r].isFlo && reg[r].val._int > 0) {
            reg[PC_REG].val._int = m;
        }
        break;
    case opJGE:
        if (reg[r].isFlo && reg[r].val._flo >= 0) {
            reg[PC_REG].val._int = m;
        }
        else if (!reg[r].isFlo && reg[r].val._int >= 0) {
            reg[PC_REG].val._int = m;
        }
        break;
    case opJEQ:
        if (reg[r].isFlo && reg[r].val._flo == 0) {
            reg[PC_REG].val._int = m;
        }
        else if (!reg[r].isFlo && reg[r].val._int == 0) {
            reg[PC_REG].val._int = m;
        }
        break;
    case opJNE:
        if (reg[r].isFlo && reg[r].val._flo != 0) {
            reg[PC_REG].val._int = m;
        }
        else if (!reg[r].isFlo && reg[r].val._int != 0) {
            reg[PC_REG].val._int = m;
        }
        break;

        /* end of legal instructions */
    } /* case */
    return srOKAY;
} /* stepTM */

/********************************************/
int doCommand(void)
{
    char cmd;
    int stepcnt = 0, i;
    int printcnt;
    int stepResult;
    int regNo, loc;
    do
    {
//        printf("Enter command: ");
//        fflush(stdin);
//        fflush(stdout);
//        gets_s(in_Line);
        input_my(in_Line);
        lineLen = strlen(in_Line);
        inCol = 0;
    } while (!getWord());

    cmd = word[0];
    switch (cmd)
    {
    case 't':
        /***********************************/
        traceflag = !traceflag;
        printf("Tracing now ");
        simshow("Tracing now ");
        if (traceflag) { printf("on.\n");simshow("on.\n");} else{ printf("off.\n");
            simshow("off.\n");}
        break;

    case 'h':
        /***********************************/
        printf("Commands are:\n");
                    simshow("Commands are:\n");
                    printf("   s(tep <n>      "\
                        "Execute n (default 1) TM instructions\n");
                    simshow("   s(tep <n>      "\
                            "Execute n (default 1) TM instructions\n");
                    printf("   g(o            "\
                        "Execute TM instructions until HALT\n");
                    simshow("   g(o            "\
                            "Execute TM instructions until HALT\n");
                    printf("   r(egs          "\
                        "Print the contents of the registers\n");
                    simshow("   r(egs          "\
                            "Print the contents of the registers\n");
                    printf("   i(Mem <b <n>>  "\
                        "Print n iMem locations starting at b\n");
                    simshow("   i(Mem <b <n>>  "\
                            "Print n iMem locations starting at b\n");
                    printf("   d(Mem <b <n>>  "\
                        "Print n dMem locations starting at b\n");
                    simshow("   d(Mem <b <n>>  "\
                            "Print n dMem locations starting at b\n");
                    printf("   t(race         "\
                        "Toggle instruction trace\n");
                    simshow("   t(race         "\
                            "Toggle instruction trace\n");
                    printf("   p(rint         "\
                        "Toggle print of total instructions executed"\
                        " ('go' only)\n");
                    simshow("   p(rint         "\
                            "Toggle print of total instructions executed"\
                            " ('go' only)\n");
                    printf("   c(lear         "\
                        "Reset simulator for new execution of program\n");
                    simshow("   c(lear         "\
                            "Reset simulator for new execution of program\n");
                    printf("   h(elp          "\
                        "Cause this list of commands to be printed\n");
                    simshow("   h(elp          "\
                            "Cause this list of commands to be printed\n");
                    printf("   q(uit          "\
                        "Terminate the simulation\n");
                    simshow("   q(uit          "\
                            "Terminate the simulation\n");
        break;

    case 'p':
        /***********************************/
        icountflag = !icountflag;
        printf("Printing instruction count now ");
                    simshow("Printing instruction count now ");
                    if (icountflag) {printf("on.\n");
                    simshow("on.\n");} else {printf("off.\n");
                    simshow("off.\n");}
        break;

    case 's':
        /***********************************/
        if (atEOL()) {
            stepcnt = 1;
        }
        else if (getNum()) {
            if (!isFloat) {
                stepcnt = abs((int)num);
            }
            else {
                printf("Error：Integer expected\n");
                simshow("Error：Integer expected\n");
                break;
            }
        }
        else  {printf("Step count?\n");simshow("Step count?\n");}
        break;

    case 'g':   stepcnt = 1;     break;

    case 'r':
        /***********************************/
        for (i = 0; i < NO_REGS; i++)
        {
            if (!reg[i].isFlo) {
                printf("%1d: %4d    ", i, reg[i].val._int);
                tqstr=QString("%1: %2    ").arg(i,1,10,QLatin1Char(' ')).
                                            arg(reg[i].val._int,4,10,QLatin1Char(' '));
                                    simshow(tqstr);
            }
            else {
                printf("%1d: %f    ", i, reg[i].val._flo);
                tqstr=QString("%1: %2    ").arg(i,1,10,QLatin1Char(' ')).
                                            arg(reg[i].val._flo);
                                    simshow(tqstr);
            }
            if ((i % 4) == 3)  {printf("\n");simshow("\n");}
        }
        break;

    case 'i':
        /***********************************/
        printcnt = 1;
        if (getNum())
        {
            if (!isFloat) {
                iloc = num;
                if (getNum()) {
                    if (!isFloat) {
                        printcnt = num;
                    }
                    else {
                        printf("Error：Integer expected！\n");
                        simshow("Error：Integer expected！\n");
                        break;
                    }
                }
            }
            else {
                printf("Error：Integer expected！\n");
                simshow("Error：Integer expected！\n");
                break;
            }
        }
        if (!atEOL())
           { printf("Instruction locations?\n");
        simshow("Instruction locations?\n");}
        else
        {
            while ((iloc >= 0) && (iloc < IADDR_SIZE)
                && (printcnt > 0))
            {
                writeInstruction(iloc);
                iloc++;
                printcnt--;
            }
        }
        break;

    case 'd':
        /***********************************/
        printcnt = 1;
        if (getNum())
        {
            if (!isFloat) {
                dloc = num;
                if (getNum()) {
                    if (!isFloat) {
                        printcnt = num;
                    }
                    else {
                        printf("Error：Integer expected！\n");
                        simshow("Error：Integer expected！\n");
                        break;
                    }
                }
            }
            else {
                printf("Error：Integer expected！\n");
                simshow("Error：Integer expected！\n");
                break;
            }
        }
        if (!atEOL()){
            printf("Data locations?\n");
            simshow("Data locations?\n");}
        else
        {
            while ((dloc >= 0) && (dloc < DADDR_SIZE)
                && (printcnt > 0))
            {
                if (dMem[dloc].isFlo) {
                    printf("%5d: %f\n", dloc, dMem[dloc].val._flo);
                    tqstr=QString("%1: %2\n").arg(dloc,5,10,QLatin1Char(' ')).
                                                    arg(dMem[dloc].val._flo);
                                            simshow(tqstr);
                }
                else {
                    printf("%5d: %5d\n", dloc, dMem[dloc].val._int);
                    tqstr=QString("%1: %2\n").arg(dloc,5,10,QLatin1Char(' ')).
                                                    arg(dMem[dloc].val._int,5,10,QLatin1Char(' '));
                                            simshow(tqstr);
                }
                dloc++;
                printcnt--;
            }
        }
        break;

    case 'c':
        /***********************************/
        iloc = 0;
        dloc = 0;
        stepcnt = 0;
        for (regNo = 0; regNo < NO_REGS; regNo++) {
            reg[regNo].isFlo = FALSE;
            reg[regNo].val._int = 0;
        }
        dMem[0].isFlo = FALSE;
        dMem[0].val._int = DADDR_SIZE - 1;
        for (loc = 1; loc < DADDR_SIZE; loc++) {
            dMem[loc].isFlo = FALSE;
            dMem[loc].val._int = 0;
        }
        break;

    case 'q': return FALSE;  /* break; */

    default: printf("Command %c unknown.\n", cmd);
        tqstr=QString("Command %1 unknown.\n").arg(cmd);
        simshow(tqstr);break;
    }  /* case */
    stepResult = srOKAY;
    if (stepcnt > 0)
    {
        if (cmd == 'g')
        {
            stepcnt = 0;
            while (stepResult == srOKAY)
            {
                iloc = reg[PC_REG].val._int;
                if (traceflag) writeInstruction(iloc);
                stepResult = stepTM();
                stepcnt++;
            }
            if (icountflag){
                printf("Number of instructions executed = %d\n", stepcnt);
              tqstr=QString("Number of instructions executed = %1\n").arg(stepcnt);
                                    simshow(tqstr);}
        }
        else
        {
            while ((stepcnt > 0) && (stepResult == srOKAY))
            {
                iloc = reg[PC_REG].val._int;
                if (traceflag) writeInstruction(iloc);
                stepResult = stepTM();
                stepcnt--;
            }
        }
        printf("%s\n", stepResultTab[stepResult]);
        tqstr=QString("%1\n").arg(stepResultTab[stepResult]);
                simshow(tqstr);
    }
    return TRUE;
} /* doCommand */


/***************/
/* 解释器主接口*/
/***************/

void simulate(const char* pgmName)
{
    pgm = fopen(pgmName, "r");
    if (pgm == NULL)
    {
        printf("file '%s' not found\n", pgmName);
        QString qstr = "file '"+QString(pgmName)+"' not found\n";
                QMessageBox::about(NULL,"提示",qstr);
        exit(1);
    }

    /* read the program */
    if (!readInstructions())
        exit(1);
    tmform=new tmwid;
        tmform->show();
    /* switch input file to terminal */
    /* reset( input ); */
    /* read-eval-print */
//    printf("TM  simulation (enter h for help)...\n");
//    do
//        done = !doCommand();
//    while (!done);
//    printf("Simulation done.\n");
//    fclose(pgm);
}
