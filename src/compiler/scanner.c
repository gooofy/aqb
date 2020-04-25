#include <ctype.h>
#include <stdint.h>
#include <math.h>

#include "scanner.h"
#include "hashmap.h"
#include "util.h"

static FILE *g_fin=NULL;

static char  g_ch;
static bool  g_eof = TRUE;
static int   g_line, g_col;
static bool  g_eol = FALSE;
static bool  g_verbose = TRUE;
static map_t g_tokens = NULL;
static int   S_line, S_col;

// scanner globals available to other modules

int    S_token, S_inum;
double S_fnum;
char   S_str[S_MAX_STRING];
char   S_strlc[S_MAX_STRING]; // S_str converted to lower case

static void init_tokens(void)
{
    if (g_tokens)
        return;

    g_tokens = hashmap_new();
    hashmap_put (g_tokens, "print",    (void *) S_PRINT);
    hashmap_put (g_tokens, "option",   (void *) S_OPTION);
    hashmap_put (g_tokens, "sub",      (void *) S_SUB);
    hashmap_put (g_tokens, "function", (void *) S_FUNCTION);
    hashmap_put (g_tokens, "for",      (void *) S_FOR);
    hashmap_put (g_tokens, "to",       (void *) S_TO);
    hashmap_put (g_tokens, "step",     (void *) S_STEP);
    hashmap_put (g_tokens, "next",     (void *) S_NEXT);
    hashmap_put (g_tokens, "xor",      (void *) S_XOR);
    hashmap_put (g_tokens, "eqv",      (void *) S_EQV);
    hashmap_put (g_tokens, "imp",      (void *) S_IMP);
    hashmap_put (g_tokens, "or",       (void *) S_OR);
    hashmap_put (g_tokens, "and",      (void *) S_AND);
    hashmap_put (g_tokens, "not",      (void *) S_NOT);
    hashmap_put (g_tokens, "mod",      (void *) S_MOD);
    hashmap_put (g_tokens, "if",       (void *) S_IF);
    hashmap_put (g_tokens, "then",     (void *) S_THEN);
    hashmap_put (g_tokens, "end",      (void *) S_END);
    hashmap_put (g_tokens, "endif",    (void *) S_ENDIF);
    hashmap_put (g_tokens, "else",     (void *) S_ELSE);
    hashmap_put (g_tokens, "elseif",   (void *) S_ELSEIF);
    hashmap_put (g_tokens, "goto",     (void *) S_GOTO);
    hashmap_put (g_tokens, "byval",    (void *) S_BYVAL);
    hashmap_put (g_tokens, "byref",    (void *) S_BYREF);
    hashmap_put (g_tokens, "as",       (void *) S_AS);
    hashmap_put (g_tokens, "static",   (void *) S_STATIC);
    hashmap_put (g_tokens, "declare",  (void *) S_DECLARE);
    hashmap_put (g_tokens, "let",      (void *) S_LET);
    hashmap_put (g_tokens, "window",   (void *) S_WINDOW);
    hashmap_put (g_tokens, "close",    (void *) S_CLOSE);
    hashmap_put (g_tokens, "on",       (void *) S_ON);
    hashmap_put (g_tokens, "off",      (void *) S_OFF);
    hashmap_put (g_tokens, "stop",     (void *) S_STOP);
    hashmap_put (g_tokens, "input",    (void *) S_INPUT);
    hashmap_put (g_tokens, "line",     (void *) S_LINE);
    hashmap_put (g_tokens, "true",     (void *) S_TRUE);
    hashmap_put (g_tokens, "false",    (void *) S_FALSE);
    hashmap_put (g_tokens, "shared",   (void *) S_SHARED);
    hashmap_put (g_tokens, "dim",      (void *) S_DIM);
    hashmap_put (g_tokens, "assert",   (void *) S_ASSERT);
    hashmap_put (g_tokens, "menu",     (void *) S_MENU);
    hashmap_put (g_tokens, "gadget",   (void *) S_GADGET);
    hashmap_put (g_tokens, "mouse",    (void *) S_MOUSE);
    hashmap_put (g_tokens, "call",     (void *) S_CALL);
}

A_pos S_getpos(void)
{
    return (S_col << 16) | S_line;
}
int S_getcol(A_pos pos)
{
    return pos >> 16;
}
int S_getline(A_pos pos)
{
    return pos & 0xffff;
}

static void getch(void)
{
    int n;

    if (g_eol)
    {
        g_eol = FALSE;
        g_col = 1;
        g_line++;
    }
    else
    {
        g_col++;
    }

    n = fread(&g_ch, 1, 1, g_fin);
    if (n<1)
    {
        g_eof = TRUE;
    }
    else
    {
        if (g_verbose)
            printf("%c", g_ch);
        if (g_ch == '\n')
            g_eol = TRUE;
    }
}

void S_init(FILE *fin)
{
    init_tokens();
    g_fin   = fin;
    g_eof   = FALSE;
    g_eol   = FALSE;
    g_line  = 1;
    g_col   = 1;
    S_token = S_ERROR;
    S_inum  = 0;
    getch();
    S_getsym();
}

static bool is_whitespace(void)
{
    return (g_ch == ' ') || (g_ch == '\t') || (g_ch == '\r');
}

static bool is_idstart(void)
{
    return ((g_ch >= 'a') && (g_ch <= 'z')) || ((g_ch >= 'A') && (g_ch <= 'Z')) ;
}

static bool is_digit(void)
{
    return (g_ch>='0') && (g_ch<='9');
}

static bool is_idcont(void)
{
    return ((g_ch >= 'a') && (g_ch <= 'z')) || ((g_ch >= 'A') && (g_ch <= 'Z')) || is_digit() || g_ch=='.' ;
}

static void number(bool negative)
{
    S_inum = 0;
    while (isdigit(g_ch)) {
        S_inum = (g_ch - '0') + S_inum*10;
        getch();
    }
    S_token = S_INUM;
    if (negative)
        S_inum *= -1;
    if (g_ch == '!')
    {
        getch();
        S_token = S_FNUM;
        S_fnum = S_inum;
    }
    else
    {
        if (g_ch == '.')
        {
            double m = 0.1;
            S_token = S_FNUM;
            S_fnum = S_inum;
            getch();
            while (isdigit(g_ch)) {
                S_fnum += ((double)(g_ch - '0')) * m;
                m /= 10.0;
                getch();
            }
        }
        if ( (g_ch == 'e') || (g_ch == 'E') )
        {
            bool negative = FALSE;
            if (S_token == S_INUM)
            {
                S_token = S_FNUM;
                S_fnum = S_inum;
            }
            getch();
            if (g_ch=='-')
            {
                negative = TRUE;
                getch();
            }
            else
            {
               if (g_ch=='+')
                getch();
            }
            int e = 0;
            while (isdigit(g_ch)) {
                e = (g_ch - '0') + e*10;
                getch();
            }
            if (negative)
                e = -1 * e;
            S_fnum *= pow(10, e);
        }
    }
}

int S_identifier(void)
{
    int l = 0;
    int t;
    S_token = S_IDENT;

    S_str[l] = g_ch;
    S_strlc[l] = tolower(g_ch);
    l++;
    getch();
    while (is_idcont() && !g_eof)
    {
        S_str[l] = g_ch;
        S_strlc[l] = tolower(g_ch);
        l++;
        getch();
    }

    // type marker?
    switch (g_ch)
    {
        case '%':
        case '&':
        case '!':
        case '#':
        case '$':
            S_str[l] = g_ch;
            S_strlc[l] = g_ch;
            l++;
            getch();
            break;
    }
    S_str[l] = '\0';
    S_strlc[l] = '\0';
    // is this a known token?
    if (hashmap_get(g_tokens, S_strlc, (any_t *)&t) == MAP_OK)
        S_token = t;
    if (g_verbose)
        printf("[%d %s]", S_token, S_str);
    return S_token;
}

int S_getsym(void)
{
    // skip whitespace, line continuations
    while ((is_whitespace() || g_ch=='_') && !g_eof)
    {
        if (g_ch=='_')
        {
            getch();
            if (g_ch == '\r')
                getch();
            if (g_ch != '\n')
                break;
        }
        else
        {
            getch();
        }
    }

    // skip line comments
    if (!g_eof && (g_ch == '\''))
    {
        getch();
        while (!g_eof && (g_ch != '\n'))
            getch();
    }

    if (g_eof)
    {
        S_token = S_EOF;
        return S_token;
    }

    S_line = g_line; S_col = g_col;

    if (is_idstart())
    {
        return S_identifier();
    }
    if (is_digit())
    {
        number (FALSE);
        if (g_verbose)
            printf("[%d %d]", S_token, S_inum);
        return S_token;
    }

    switch (g_ch)
    {
        case '"':
        {
            int l = 0;
            S_token = S_STRING;
            getch();

            while ( (g_ch != '"') && !g_eof )
            {
                S_str[l] = g_ch;
                l++;
                getch();
            }
            if (g_ch == '"')
                getch();
            S_str[l] = '\0';
            S_strlc[l] = '\0';

            break;
        }
        case '\n':
            S_token = S_EOL;
            getch();
            break;
        case ':':
            S_token = S_COLON;
            getch();
            break;
        case ';':
            S_token = S_SEMICOLON;
            getch();
            break;
        case ',':
            S_token = S_COMMA;
            getch();
            break;
        case '(':
            S_token = S_LPAREN;
            getch();
            break;
        case ')':
            S_token = S_RPAREN;
            getch();
            break;
        case '=':
            S_token = S_EQUALS;
            getch();
            break;
        case '-':
            S_token = S_MINUS;
            getch();
            if (g_ch == '>')
            {
                S_token = S_POINTER;
                getch();
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            number(FALSE);
            break;
        case '^':
            S_token = S_EXP;
            getch();
            break;
        case '*':
            S_token = S_ASTERISK;
            getch();
            break;
        case '/':
            S_token = S_SLASH;
            getch();
            break;
        case '\\':
            S_token = S_BACKSLASH;
            getch();
            break;
        case '+':
            S_token = S_PLUS;
            getch();
            break;
        case '.':
            S_token = S_PERIOD;
            getch();
            break;
        case '>':
            S_token = S_GREATER;
            getch();
            if (g_ch == '=')
            {
                S_token = S_GREATEREQ;
                getch();
            }
            break;
        case '<':
            S_token = S_LESS;
            getch();
            if (g_ch == '=')
            {
                S_token = S_LESSEQ;
                getch();
            }
            else
            {
                if (g_ch == '>')
                {
                    S_token = S_NOTEQ;
                    getch();
                }
            }
            break;
        default:
            S_token = S_ERROR;
            getch();
    }

    if (g_verbose)
        printf("[%d]", S_token);
    return S_token;
}


