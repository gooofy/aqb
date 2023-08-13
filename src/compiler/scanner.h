#ifndef HAVE_SCANNER_H
#define HAVE_SCANNER_H

#include <stdio.h>
#include <stdbool.h>

#include "symbol.h"

#define MAX_LINE_LEN   8192

typedef enum {
    S_EOF, S_IDENT, S_STRING,
    S_PERIOD, S_SEMICOLON, S_COLON, S_COMMA, S_ASTERISK,
    S_LPAREN, S_RPAREN, S_LBRACE, S_RBRACE, S_LBRACKET, S_RBRACKET, 
    S_EQUALS, S_LESS,
    S_SLASH

    //S_ERRTKN, S_EOL,
    //S_IDENT, S_STRING, S_COLON, S_SEMICOLON, S_COMMA, S_INUM, S_FNUM, S_MINUS,
    //S_LPAREN, S_RPAREN, S_EQUALS, S_EXP, S_ASTERISK, S_SLASH, S_BACKSLASH,
    //S_PLUS, S_GREATER, S_LESS, S_HASH, S_NOTEQ, S_LESSEQ, S_GREATEREQ, S_POINTER,
    //S_PERIOD, S_AT, S_LBRACKET, S_RBRACKET, S_TRIPLEDOTS

} S_token;

typedef struct { uint16_t col; uint16_t line; } S_pos;

extern S_pos S_noPos;

typedef enum { S_thNone, S_thSingle, S_thDouble, S_thInteger, S_thLong, S_thUInteger, S_thULong } S_typeHint;

typedef struct
{
    S_pos   pos;
    S_token kind;
    union
    {
        struct {
            int32_t    inum;
            double     fnum;
            S_typeHint typeHint;
        }                             literal;
        S_symbol                      sym;
        char                         *str;
    } u;
} S_tkn_t;

extern S_tkn_t S_tkn; 

static inline bool S_isWhitespace(char ch)
{
    return (ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n');
}

static inline bool S_isIDStart(char ch)
{
    return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || (ch == '_') ;
}

static inline bool S_isDigit(char ch)
{
    return (ch>='0') && (ch<='9');
}

static inline bool S_isIDCont(char ch)
{
    return ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || ch == '_' || S_isDigit(ch) ;
}

bool    S_nextToken (void);

string  S_getSourceLine (int line);

void    S_init(const char *sourcefn, FILE *sourcef);

#endif
