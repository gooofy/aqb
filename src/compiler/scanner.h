#ifndef HAVE_SCANNER_H
#define HAVE_SCANNER_H

#include <stdio.h>

#define S_EOF        0
#define S_ERROR      1
#define S_EOL        2

#define S_IDENT     10
#define S_STRING    11
#define S_COLON     12
#define S_SEMICOLON 13
#define S_COMMA     14
#define S_INUM      15
#define S_MINUS     16
#define S_LPAREN    17
#define S_RPAREN    18
#define S_EQUALS    19
#define S_EXP       20
#define S_ASTERISK  21
#define S_SLASH     22
#define S_BACKSLASH 23
#define S_PLUS      24
#define S_GREATER   25
#define S_LESS      26
#define S_NOTEQ     27
#define S_LESSEQ    28
#define S_GREATEREQ 29

#define S_PRINT    100
#define S_OPTION   101
#define S_SUB      102
#define S_FUNCTION 103
#define S_FOR      104
#define S_TO       105
#define S_STEP     106
#define S_NEXT     107
#define S_XOR      108
#define S_EQV      109
#define S_IMP      110
#define S_OR       111
#define S_AND      112
#define S_NOT      113
#define S_MOD      114
#define S_IF       115
#define S_THEN     116
#define S_END      117
#define S_ENDIF    118
#define S_ELSE     119
#define S_ELSEIF   120
#define S_GOTO     121
#define S_BYVAL    122
#define S_BYREF    123
#define S_AS       124
#define S_STATIC   125
#define S_DECLARE  126
#define S_LET      127

#define S_MAX_STRING 1024

typedef int A_pos; // 32 bits: cccc cccc llll llll

extern int  S_token, S_inum;
extern char S_str[S_MAX_STRING];
extern char S_strlc[S_MAX_STRING]; // S_str converted to lower case

void S_init(FILE *fin);

int S_getsym(void);

A_pos S_getpos(void);
int S_getline(A_pos pos);
int S_getcol(A_pos pos);

#endif
