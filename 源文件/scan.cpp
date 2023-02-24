/*********************************/
/*SCAN.cpp                       */
/*版本：2.0                      */
/*作者：Liji Chan                */
/*接口函数：getToken()           */
/*功能：实现对源文件的词法分析， */
/*      返回标记和标记的类型     */
/*********************************/

#include<iostream>
#include "GLOBAL.h"
#include "SCAN.h"
#include <QString>
#include<stdio.h>
#include<QMessageBox>
#include<QFile>
#include<QByteArray>
using namespace std;
extern QFile file;
//QTextStream in(&file);
//QString line = in.readLine();

/* 保留字数组*/
static struct {
    string str;
    TokenType _type;
}reservedWords[] = { {"if", IF},{"else", ELSE},{"int", INT},{"float", FLOAT},
                    {"return", RETURN},{"void", VOID},{"do", DO},  {"while", WHILE},
                    {"INPUT", INPUT}, {"OUTPUT", OUTPUT} };

static const int BUFLEN = 256; /* 缓存数组最大长度*/
static const int MAXRESERVEDLEN = 10; /* 保留字数组最大长度*/
const int MAXTOKENLEN = 40; /* 标识符最大长度*/



static char lineBuf[BUFLEN]; /* 缓存数组*/
char TokenString[MAXTOKENLEN + 1]; /* 存放token的数组*/
int lineno = 0; /* 源文件当前读取到的行*/
int linepos = 0; /* 缓存数组当前下标*/
int bufsize = 0; /* 缓存数组当前大小*/
bool EOF_flag = false; /* 标志是否读取到文件末尾*/


/* 读取下一个字符*/
char getNextChar() {
    if (!(linepos < bufsize)) {
        if (fgets(lineBuf, BUFLEN - 1, source)) {
                    bufsize = strlen(lineBuf);
                    lineno++;
//                    QMessageBox::about(NULL,"提示",QString(lineBuf));
                }
        else {
            EOF_flag = true;
            lineBuf[0] = EOF;
            QMessageBox::about(NULL,"提示","完毕");
            lineno = 0;
        }
        linepos = 0;
    }
    return lineBuf[linepos++];
}

/* 回溯一个字符*/
inline void ungetNextChar() {
    linepos--;
}

/* 保留字检查*/
TokenType reservedWordsLookUp(const char* tokenString) {
    for (int i = 0; i < MAXRESERVEDLEN; i++) {
        if (!strcmp(reservedWords[i].str.c_str(), tokenString)) {
            return reservedWords[i]._type;
        }
    }
    return ID;
}

/* 获取下一个token，返回其类型，
   运作原理是一个有穷状态机 */
TokenType getToken() {
    StateType state = START;
    TokenType tokenType;
    bool save;
    int tokenLen = 0;
    while (state != DONE) {
        unsigned char c = getNextChar();
        save = true;
        switch (state) {
        case START:
            if (isdigit(c)) {
                tokenType = NUM_INT;
                state = INNUM;
                break;
            }
            else if (isalpha(c) || c == '_') {
                tokenType = ID;
                state = INID;
                break;
            }
            else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                save = false;
                state = START;
                break;
            }
            else if ((char)c == EOF) {
                tokenType = ENDFILE;
                save = false;
                state = DONE;
                break;
            }
            else {
                switch (c) {
                case '{':tokenType = LPAREN_1; state = DONE; break;
                case '[':tokenType = LPAREN_2; state = DONE; break;
                case '(':tokenType = LPAREN_3; state = DONE; break;
                case '}':tokenType = RPAREN_1; state = DONE; break;
                case ']':tokenType = RPAREN_2; state = DONE; break;
                case ')':tokenType = RPAREN_3; state = DONE; break;
                case '+':tokenType = PLUS; state = DONE; break;
                case '-':tokenType = MINUS; state = DONE; break;
                case '*':tokenType = MULTI; state = DONE; break;
                case '%':tokenType = MOD; state = DONE; break;
                case ',':tokenType = COMM; state = DONE; break;
                case ';':tokenType = SEMI; state = DONE; break;
                case '<':tokenType = LT; state = INOP; break;
                case '>':tokenType = RT; state = INOP; break;
                case '=':tokenType = ASSIGN; state = INOP; break;
                case '!':tokenType = UNEQ; state = INOP; break;
                case '/':tokenType = DIVI; state = INOP; break;
                default:
                    /* 遇到不可识别的字符*/
                    printf("Token error on (line %d, pos %d).\nPlease check it!\n", lineno, linepos);
                    QMessageBox::about(NULL,"提示","Token error on (line "+QString::number(lineno-12)+", pos "
                                                           +QString::number(linepos)+").\nPlease check it!\n");
                    lineno = 0;
                    save = false;
                    tokenType = ERROR;
                    state = DONE;
                    break;
                }
                break;
            }
        case INNUM:
            if (!isdigit(c)) {
                if (tokenType == NUM_INT) {
                    if (c == '.') { tokenType = NUM_FLOAT; }
                    else { save = false; ungetNextChar(); state = DONE; }
                }
                else { save = false; ungetNextChar(); state = DONE; }
            }
            break;
        case INID:
            if (!isalpha(c) && !isdigit(c) && c != '_') {
                save = false;
                ungetNextChar();
                state = DONE;
            }
            break;
        case INOP:
            switch (tokenType) {
            case LT:
                if (c != '=') { save = false; ungetNextChar(); }
                else { tokenType = LT_EQ; }
                state = DONE;
                break;
            case RT:
                if (c != '=') { save = false; ungetNextChar(); }
                else { tokenType = RT_EQ; }
                state = DONE;
                break;
            case ASSIGN:
                if (c != '=') { save = false; ungetNextChar(); }
                else { tokenType = EQ; }
                state = DONE;
                break;
            case UNEQ:
                if (c != '=') { save = false; ungetNextChar(); tokenType = ERROR; }
                state = DONE;
                break;
            case DIVI:
                if (c != '*') { save = false; ungetNextChar(); state = DONE; }
                else { state = INCOMMENT; }
                break;
            }
            break;
        case INCOMMENT:
            if (tokenType == MULTI && c == '/') {
                save = false;
                tokenLen = 0;
                state = START;
            }
            else if (c == '*') { tokenType = MULTI; }
            else { tokenType = DIVI; }
            break;
        case DONE:
            break;
        }
        /* 字符在本轮有效，且不超过最大长度时添加到数组末尾*/
        if (save && tokenLen < MAXTOKENLEN) {
            TokenString[tokenLen++] = c;
        }
    }
    TokenString[tokenLen] = '\0';
    /* 保留字检查*/
    if (tokenType == ID) { tokenType = reservedWordsLookUp(TokenString); }
    return tokenType;
}

