#ifndef HAVE_SCANNER_H
#define HAVE_SCANNER_H

#include <stdio.h>
#include <stdbool.h>

#include "symbol.h"

#define MAX_LINE_LEN   8192

typedef enum {
    S_EOF, S_IDENT, S_STRING, S_INUM,
    S_PERIOD,       // .
    S_SEMICOLON,    // ;
    S_COLON,        // :
    S_COMMA,        // ,
    S_LPAREN,       // (
    S_RPAREN,       // )
    S_LBRACE,       // {
    S_RBRACE,       // }
    S_LBRACKET,     // [
    S_RBRACKET,     // ]
    S_EQUALS,       // =
    S_EEQUALS,      // ==
    S_NEQUALS,      // !=
    S_LESS,         // <
    S_LESSEQ,       // <=
    S_GREATER,      // >
    S_GREATEREQ,    // >=
    S_AND,          // &
    S_LAND,         // &&
    S_OR,           // |
    S_LOR,          // ||
    S_EOR,          // ^
    S_NOT,          // !
    S_SLASH,        // /
    S_QUESTIONMARK, // ?
    S_PLUS,         // +
    S_MINUS,        // -
    S_ASTERISK,     // *
    S_MOD,          // %
    S_NEG,          // ~
    S_LSHIFT,       // <<
    S_RSHIFT,       // >>
    S_PLUSEQUALS,   // +=
    S_MINUSEQUALS,  // -=
    S_MULEQUALS,    // *=
    S_DIVEQUALS,    // /=
    S_MODEQUALS,    // %=
    S_ANDEQUALS,    // &=
    S_OREQUALS,     // |=
    S_EOREQUALS,    // ^=
    S_LSHIFTEQUALS, // <<=
    S_RSHIFTEQUALS, // >>=
    S_PLUSPLUS,     // ++
    S_MINUSMINUS,   // --

    // C# keywords
    S_TRUE,
    S_FALSE,
    S_USING,
    S_NAMESPACE,
    S_NEW,
    S_PUBLIC,
    S_PROTECTED,
    S_INTERNAL,
    S_PRIVATE,
    S_ABSTRACT,
    S_SEALED,
    S_STATIC,
    S_READONLY,
    S_UNSAFE,
    S_REF,
    S_PARTIAL,
    S_CLASS,
    S_STRUCT,
    S_INTERFACE,
    S_ENUM,
    S_DELEGATE,
    S_WHERE,
    S_EXTERN,
    S_VIRTUAL,
    S_OVERRIDE,
    S_ASYNC,
    S_VOID,
    S___arglist,
    S_IF,
    S_SWITCH,
    S_WHILE,
    S_DO,
    S_FOR,
    S_FOREACH,
    S_VAR,
    S_AWAIT,
    S_IS,
    S_AS,
} S_token;

typedef struct { uint16_t col; uint16_t line; } S_pos;

extern S_pos S_noPos;

typedef enum { S_thNone, S_thSingle, S_thDouble, S_thUnsigned, S_thLong, S_thULong } S_typeHint;

typedef struct
{
    S_pos   pos;
    S_token kind;
    union
    {
        struct {
            int64_t    inum;
            double     fnum;
            S_typeHint typeHint;
        }                             literal;
        S_symbol                      sym;
        char                         *str;
    } u;
} S_tkn_t;

extern S_tkn_t S_tkn; 

typedef struct S_state_ S_state;
struct S_state_
{
    S_tkn_t       tkn;
    char          ch;
    bool          eof;
    uint16_t      line, col;
    bool          eol;
    char          str[MAX_LINE_LEN];
    char          cur_line[MAX_LINE_LEN];
    long          offset;
};

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

void    S_recordState  (S_state *s);
void    S_restoreState (S_state *s);

void    S_boot(void);

#endif
