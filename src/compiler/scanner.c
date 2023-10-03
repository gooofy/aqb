#define ENABLE_DEBUG

#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#include "scanner.h"
#include "util.h"
#include "errormsg.h"
#include "options.h"
#include "symbol.h"
#include "logger.h"

S_tkn_t S_tkn;

S_pos S_noPos = {0,0};

static const char   *g_sourcefn;
static FILE         *g_sourcef;

static char          g_ch;
static bool          g_eof = true;
static uint16_t      g_line, g_col;
static bool          g_eol = false;
static char          g_str[MAX_LINE_LEN];

static char          g_cur_line[MAX_LINE_LEN];

static TAB_table     g_src;     // line number -> string
static TAB_table     g_syms;    // S_symbol -> S_token

static void remember_pos(void)
{
    S_tkn.pos.col  = g_col;
    S_tkn.pos.line = g_line;
}

#ifdef ENABLE_DEBUG
static void _print_token(void)
{
    switch (S_tkn.kind)
    {
        case S_IDENT         : LOG_printf(LOG_DEBUG, "[IDENT %s]",  S_name(S_tkn.u.sym)); break;
        case S_STRING        : LOG_printf(LOG_DEBUG, "[STRING %s]", S_tkn.u.str); break;
        case S_INUM          : LOG_printf(LOG_DEBUG, "[INUM %d]", S_tkn.u.literal.inum); break;
        case S_EOF           : LOG_printf(LOG_DEBUG, "[EOF]"); break;
        case S_PERIOD        : LOG_printf(LOG_DEBUG, "."); break;
        case S_LBRACE        : LOG_printf(LOG_DEBUG, "{"); break;
        case S_RBRACE        : LOG_printf(LOG_DEBUG, "}"); break;
        case S_SEMICOLON     : LOG_printf(LOG_DEBUG, ";"); break;
        case S_COLON         : LOG_printf(LOG_DEBUG, ":"); break;
        case S_COMMA         : LOG_printf(LOG_DEBUG, ","); break;
        case S_ASTERISK      : LOG_printf(LOG_DEBUG, "*"); break;
        case S_QUESTIONMARK  : LOG_printf(LOG_DEBUG, "?"); break;
        case S_PLUS          : LOG_printf(LOG_DEBUG, "+"); break;
        case S_MINUS         : LOG_printf(LOG_DEBUG, "-"); break;
        case S_SLASH         : LOG_printf(LOG_DEBUG, "/"); break;
        case S_MOD           : LOG_printf(LOG_DEBUG, "%"); break;
        case S_NEG           : LOG_printf(LOG_DEBUG, "~"); break;
        case S_AND           : LOG_printf(LOG_DEBUG, "&"); break;
        case S_LAND          : LOG_printf(LOG_DEBUG, "&&"); break;
        case S_OR            : LOG_printf(LOG_DEBUG, "|"); break;
        case S_LOR           : LOG_printf(LOG_DEBUG, "||"); break;
        case S_EOR           : LOG_printf(LOG_DEBUG, "^"); break;
        case S_NOT           : LOG_printf(LOG_DEBUG, "!"); break;
        case S_LSHIFT        : LOG_printf(LOG_DEBUG, "<<"); break;
        case S_RSHIFT        : LOG_printf(LOG_DEBUG, ">>"); break;
        case S_EQUALS        : LOG_printf(LOG_DEBUG, "="); break;
        case S_EEQUALS       : LOG_printf(LOG_DEBUG, "=="); break;
        case S_NEQUALS       : LOG_printf(LOG_DEBUG, "!="); break;
        case S_LESS          : LOG_printf(LOG_DEBUG, "<"); break;
        case S_LESSEQ        : LOG_printf(LOG_DEBUG, "<="); break;
        case S_GREATER       : LOG_printf(LOG_DEBUG, ">"); break;
        case S_GREATEREQ     : LOG_printf(LOG_DEBUG, ">="); break;
        case S_LBRACKET      : LOG_printf(LOG_DEBUG, "["); break;
        case S_RBRACKET      : LOG_printf(LOG_DEBUG, "]"); break;
        case S_LPAREN        : LOG_printf(LOG_DEBUG, "("); break;
        case S_RPAREN        : LOG_printf(LOG_DEBUG, ")"); break;
        case S_PLUSEQUALS    : LOG_printf(LOG_DEBUG, "+="); break;
        case S_MINUSEQUALS   : LOG_printf(LOG_DEBUG, "-="); break;
        case S_MULEQUALS     : LOG_printf(LOG_DEBUG, "*="); break;
        case S_DIVEQUALS     : LOG_printf(LOG_DEBUG, "/="); break;
        case S_MODEQUALS     : LOG_printf(LOG_DEBUG, "%="); break;
        case S_ANDEQUALS     : LOG_printf(LOG_DEBUG, "&="); break;
        case S_OREQUALS      : LOG_printf(LOG_DEBUG, "|="); break;
        case S_EOREQUALS     : LOG_printf(LOG_DEBUG, "^="); break;
        case S_LSHIFTEQUALS  : LOG_printf(LOG_DEBUG, "<<="); break;
        case S_RSHIFTEQUALS  : LOG_printf(LOG_DEBUG, ">>="); break;
        case S_PLUSPLUS      : LOG_printf(LOG_DEBUG, "++"); break;
        case S_MINUSMINUS    : LOG_printf(LOG_DEBUG, "--"); break;
        case S_TRUE          : LOG_printf(LOG_DEBUG, "true"); break;
        case S_FALSE         : LOG_printf(LOG_DEBUG, "false"); break;
        case S_USING         : LOG_printf(LOG_DEBUG, "using"); break;
        case S_NAMESPACE     : LOG_printf(LOG_DEBUG, "namespace"); break;
        case S_NEW           : LOG_printf(LOG_DEBUG, "new"); break;
        case S_PUBLIC        : LOG_printf(LOG_DEBUG, "public"); break;
        case S_PROTECTED     : LOG_printf(LOG_DEBUG, "protected"); break;
        case S_INTERNAL      : LOG_printf(LOG_DEBUG, "internal"); break;
        case S_PRIVATE       : LOG_printf(LOG_DEBUG, "private"); break;
        case S_ABSTRACT      : LOG_printf(LOG_DEBUG, "abstract"); break;
        case S_SEALED        : LOG_printf(LOG_DEBUG, "sealed"); break;
        case S_STATIC        : LOG_printf(LOG_DEBUG, "static"); break;
        case S_READONLY      : LOG_printf(LOG_DEBUG, "readonly"); break;
        case S_UNSAFE        : LOG_printf(LOG_DEBUG, "unsafe"); break;
        case S_REF           : LOG_printf(LOG_DEBUG, "ref"); break;
        case S_PARTIAL       : LOG_printf(LOG_DEBUG, "partial"); break;
        case S_CLASS         : LOG_printf(LOG_DEBUG, "class"); break;
        case S_STRUCT        : LOG_printf(LOG_DEBUG, "struct"); break;
        case S_INTERFACE     : LOG_printf(LOG_DEBUG, "interface"); break;
        case S_ENUM          : LOG_printf(LOG_DEBUG, "enum"); break;
        case S_DELEGATE      : LOG_printf(LOG_DEBUG, "delegate"); break;
        case S_WHERE         : LOG_printf(LOG_DEBUG, "where"); break;
        case S_EXTERN        : LOG_printf(LOG_DEBUG, "extern"); break;
        case S_VIRTUAL       : LOG_printf(LOG_DEBUG, "virtual"); break;
        case S_OVERRIDE      : LOG_printf(LOG_DEBUG, "override"); break;
        case S_ASYNC         : LOG_printf(LOG_DEBUG, "async"); break;
        case S_VOID          : LOG_printf(LOG_DEBUG, "void"); break;
        case S_PARAMS        : LOG_printf(LOG_DEBUG, "params"); break;
        case S_IF            : LOG_printf(LOG_DEBUG, "if"); break;
        case S_SWITCH        : LOG_printf(LOG_DEBUG, "switch"); break;
        case S_WHILE         : LOG_printf(LOG_DEBUG, "while"); break;
        case S_DO            : LOG_printf(LOG_DEBUG, "do"); break;
        case S_FOR           : LOG_printf(LOG_DEBUG, "for"); break;
        case S_FOREACH       : LOG_printf(LOG_DEBUG, "foreach"); break;
        case S_VAR           : LOG_printf(LOG_DEBUG, "var"); break;
        case S_AWAIT         : LOG_printf(LOG_DEBUG, "await"); break;
        case S_IS            : LOG_printf(LOG_DEBUG, "is"); break;
        case S_AS            : LOG_printf(LOG_DEBUG, "as"); break;
    }
}
#else
static inline void _print_token(void)
{
}
#endif

static bool nextch (void)
{
    int n = fread(&g_ch, 1, 1, g_sourcef);
    if (n==1)
        return true;
    g_ch = 0;
    return false;
}

static void getch(void)
{
    if (g_eol)
    {
        g_eol = false;
        g_col = 1;
        TAB_enter (g_src, (void *) (long) g_line, String(UP_frontend, g_cur_line));
        g_line++;
    }
    else
    {
        g_col++;
    }

    g_eof = !nextch();
    if (!g_eof)
    {
        if (g_ch == '\n')
        {
            g_eol = true;
        }
        else
        {
            if (g_col<MAX_LINE_LEN)
            {
                g_cur_line[g_col-1] = g_ch;
                g_cur_line[g_col]   = 0;
            }
        }
    }
}

static bool get_digit(int *digit, int base)
{
    char ch = toupper(g_ch);
    int d;

    if ((ch>='0') && (ch<='9'))
        d = ch-'0';
    else
        if ((ch>='A') && (ch<='F'))
            d = ch-'A'+10;
        else
            return false;
    if (d>=base)
        return false;
    *digit = d;
    return true;
}

static void _number(void)
{
    int     base = 10;

    S_tkn.kind = S_INUM;

    S_tkn.u.literal.inum = 0;

    int d=0;
    get_digit(&d, base);
    getch();

    // 0x ?
    if (!d && (g_ch == 'x'))
    {
        base = 16;
        getch();
    }
    else
    {
        S_tkn.u.literal.inum = d;
    }

    while (get_digit(&d, base))
    {
        S_tkn.u.literal.inum = d + S_tkn.u.literal.inum*base;
        getch();
    }

    if (g_ch == '.')
    {
        assert (false); // FIXME
        //double m = 1.0 / base;
        //tkn->kind = S_FNUM;
        //tkn->u.literal.fnum = tkn->u.literal.inum;
        //if (!dp)
        //    getch();
        //while (get_digit(&d, base))
        //{
        //    tkn->u.literal.fnum += ((double) d) * m;
        //    m /= base;
        //    getch();
        //}
    }
    if ( (g_ch == 'e') || (g_ch == 'E') )
    {
        assert (false); // FIXME
        //bool negative = false;
        //if (tkn->kind == S_INUM)
        //{
        //    tkn->kind = S_FNUM;
        //    tkn->u.literal.fnum = tkn->u.literal.inum;
        //}
        //getch();
        //if (g_ch=='-')
        //{
        //    negative = true;
        //    getch();
        //}
        //else
        //{
        //   if (g_ch=='+')
        //    getch();
        //}
        //int e = 0;
        //while (get_digit(&d, base))
        //{
        //    e = d + e*base;
        //    getch();
        //}
        //if (negative)
        //    e = -1 * e;
        //tkn->u.literal.fnum *= pow(base, e);
    }

    // type suffix?
    switch (g_ch)
    {
        case 'u':
        case 'U':
            S_tkn.u.literal.typeHint = S_thUnsigned;
            getch();
            if ((g_ch=='l') || (g_ch=='L'))
            {
                S_tkn.u.literal.typeHint = S_thULong;
                getch();
            }
            break;
        case 'L':
        case 'l':
            S_tkn.u.literal.typeHint = S_thLong;
            getch();
            if ((g_ch=='u') || (g_ch=='U'))
            {
                S_tkn.u.literal.typeHint = S_thULong;
                getch();
            }
            break;
        default:
            S_tkn.u.literal.typeHint = S_thNone;
    }
}

static void _identifier(void)
{
    int l = 0;

    S_tkn.kind = S_IDENT;

    while (true)
    {
        g_str[l] = g_ch;
        l++;
        getch();
        if (!S_isIDCont(g_ch) || g_eof)
            break;
    }

    g_str[l] = '\0';

    S_tkn.u.sym = S_Symbol(g_str);

    // known id ?
    void *sid = TAB_look (g_syms, S_tkn.u.sym);
    if (sid)
        S_tkn.kind = (intptr_t) sid;
}

static void _string(void)
{
    int l = 0;

    S_tkn.kind = S_STRING;

    bool quoted = false;

    while (!g_eof)
    {
        if (g_ch == '\\')
        {
            quoted = true;
            getch();
            continue;
        }

        if (!quoted && (g_ch== '"'))
        {
            getch();
            break;
        }

        if (l<MAX_LINE_LEN-1)
        {
            g_str[l] = g_ch;
            l++;
        }
        getch();
        quoted = false;
    }

    g_str[l] = 0;
    S_tkn.u.str = String (UP_frontend, g_str);
}

bool S_nextToken (void)
{
    while (true)
    {
        // skip whitespace
        while (S_isWhitespace(g_ch) && !g_eof)
        {
            getch();
        }

        if (g_eof)
        {
            S_tkn.kind = S_EOF;
            return false;
        }

        // handle comments
        if (!g_eof && (g_ch == '/'))
        {
            getch();
            // FIXME: handle block comments
            if (g_eof || (g_ch != '/'))
            {
                S_tkn.kind = S_SLASH;
                goto done;
            }
            getch();
            // line comment
            while (!g_eof)
            {
                if (g_ch == '\n')
                {
                    break;
                }
                getch();
            }
            continue;
        }

        //if (g_eof)
        //    return false;

        remember_pos();

        if (S_isIDStart(g_ch))
        {
            _identifier();
            goto done;
        }
        else
        {
            if (S_isDigit(g_ch))
            {
                _number ();
                goto done;
            }
            switch (g_ch)
            {
                case '"':
                    getch();
                    _string();
                    goto done;
                case '.':
                    getch();
                    S_tkn.kind = S_PERIOD;
                    goto done;
                case '{':
                    getch();
                    S_tkn.kind = S_LBRACE;
                    goto done;
                case '}':
                    getch();
                    S_tkn.kind = S_RBRACE;
                    goto done;
                case '(':
                    getch();
                    S_tkn.kind = S_LPAREN;
                    goto done;
                case ')':
                    getch();
                    S_tkn.kind = S_RPAREN;
                    goto done;
                case '[':
                    getch();
                    S_tkn.kind = S_LBRACKET;
                    goto done;
                case ']':
                    getch();
                    S_tkn.kind = S_RBRACKET;
                    goto done;
                case ';':
                    getch();
                    S_tkn.kind = S_SEMICOLON;
                    goto done;
                case ':':
                    getch();
                    S_tkn.kind = S_COLON;
                    goto done;
                case ',':
                    getch();
                    S_tkn.kind = S_COMMA;
                    goto done;
                case '*':
                    getch();
                    S_tkn.kind = S_ASTERISK;
                    if (g_ch == '=')
                    {
                        getch();
                        S_tkn.kind = S_MULEQUALS;
                    }
                    goto done;
                case '<':
                    getch();
                    S_tkn.kind = S_LESS;
                    if (g_ch=='<')
                    {
                        getch();
                        S_tkn.kind = S_LSHIFT;
                        if (g_ch=='=')
                        {
                            getch();
                            S_tkn.kind = S_LSHIFTEQUALS;
                        }
                    }
                    else if (g_ch=='=')
                    {
                        getch();
                        S_tkn.kind = S_LESSEQ;
                    }
                    goto done;
                case '>':
                    getch();
                    S_tkn.kind = S_GREATER;
                    if (g_ch=='<')
                    {
                        getch();
                        S_tkn.kind = S_RSHIFT;
                        if (g_ch=='=')
                        {
                            getch();
                            S_tkn.kind = S_RSHIFTEQUALS;
                        }
                    }
                    else if (g_ch=='=')
                    {
                        getch();
                        S_tkn.kind = S_GREATEREQ;
                    }
                    goto done;
                case '=':
                    getch();
                    S_tkn.kind = S_EQUALS;
                    if (g_ch=='=')
                    {
                        getch();
                        S_tkn.kind = S_EEQUALS;
                    }
                    goto done;
                case '!':
                    getch();
                    S_tkn.kind = S_NOT;
                    if (g_ch=='=')
                    {
                        getch();
                        S_tkn.kind = S_NEQUALS;
                    }
                    goto done;
                case '?':
                    getch();
                    S_tkn.kind = S_QUESTIONMARK;
                    goto done;
                case '+':
                    getch();
                    S_tkn.kind = S_PLUS;
                    if (g_ch == '=')
                    {
                        getch();
                        S_tkn.kind = S_PLUSEQUALS;
                    }
                    else
                    {
                        if (g_ch == '+')
                        {
                            getch();
                            S_tkn.kind = S_PLUSPLUS;
                        }
                    }
                    goto done;
                case '-':
                    getch();
                    S_tkn.kind = S_MINUS;
                    if (g_ch == '=')
                    {
                        getch();
                        S_tkn.kind = S_MINUSEQUALS;
                    }
                    else
                    {
                        if (g_ch == '-')
                        {
                            getch();
                            S_tkn.kind = S_MINUSMINUS;
                        }
                    }
                    goto done;
                case '/':
                    getch();
                    S_tkn.kind = S_SLASH;
                    if (g_ch == '=')
                    {
                        getch();
                        S_tkn.kind = S_DIVEQUALS;
                    }
                    goto done;
                case '%':
                    getch();
                    S_tkn.kind = S_MOD;
                    if (g_ch == '=')
                    {
                        getch();
                        S_tkn.kind = S_MODEQUALS;
                    }
                    goto done;
                case '~':
                    getch();
                    S_tkn.kind = S_NEG;
                    goto done;
                case '&':
                    getch();
                    S_tkn.kind = S_AND;
                    if (g_ch == '=')
                    {
                        getch();
                        S_tkn.kind = S_ANDEQUALS;
                    }
                    else if (g_ch == '&')
                    {
                        getch();
                        S_tkn.kind = S_LAND;
                    }
                    goto done;
                case '|':
                    getch();
                    S_tkn.kind = S_OR;
                    if (g_ch == '=')
                    {
                        getch();
                        S_tkn.kind = S_OREQUALS;
                    }
                    else if (g_ch == '|')
                    {
                        getch();
                        S_tkn.kind = S_LOR;
                    }
                    goto done;
                case '^':
                    getch();
                    S_tkn.kind = S_EOR;
                    if (g_ch == '=')
                    {
                        getch();
                        S_tkn.kind = S_EOREQUALS;
                    }
                    goto done;
                default:
                    LOG_printf (LOG_ERROR, "%s:%d: lexer: invalid char '%c' (0x%02x)\n", g_sourcefn, g_line, g_ch, g_ch);
                    assert(false); // FIXME
            }
        }
    }

done:
    _print_token();

    return true;
}

void S_init(const char *sourcefn, FILE *sourcef)
{
    g_src             = TAB_empty(UP_frontend);

    g_sourcefn        = sourcefn;
    g_sourcef         = sourcef;
    g_eof             = false;
    g_eol             = false;
    g_line            = 1;
    g_col             = 0;

    g_cur_line[0]     = 0;

    getch();
    S_nextToken();
}

string  S_getSourceLine (int line)
{
    if (!line)
        return "";
    if (line == g_line)
        return g_cur_line;

    string s = TAB_look (g_src, (void*) (long) line);
    return s ? s : "";
}

void S_recordState (S_state *s)
{
    s->tkn      = S_tkn;
    s->ch       = g_ch;
    s->eof      = g_eof;
    s->line     = g_line;
    s->col      = g_col;
    s->eol      = g_eol;

    memcpy (s->str     , g_str     , MAX_LINE_LEN);
    memcpy (s->cur_line, g_cur_line, MAX_LINE_LEN);

    s->offset   = ftell (g_sourcef);
}

void S_restoreState (S_state *s)
{
    S_tkn  = s->tkn;
    g_ch   = s->ch;
    g_eof  = s->eof;
    g_line = s->line;
    g_col  = s->col;
    g_eol  = s->eol;

    memcpy (g_str     , s->str     , MAX_LINE_LEN);
    memcpy (g_cur_line, s->cur_line, MAX_LINE_LEN);

    fseek (g_sourcef, s->offset, SEEK_SET);
}

void S_boot(void)
{
    g_syms = TAB_empty (UP_symbol);

    TAB_enter (g_syms, S_Symbol("true"         ), (void *) (intptr_t) S_TRUE);
    TAB_enter (g_syms, S_Symbol("false"        ), (void *) (intptr_t) S_FALSE);
    TAB_enter (g_syms, S_Symbol("using"        ), (void *) (intptr_t) S_USING);
    TAB_enter (g_syms, S_Symbol("namespace"    ), (void *) (intptr_t) S_NAMESPACE);
    TAB_enter (g_syms, S_Symbol("new"          ), (void *) (intptr_t) S_NEW);
    TAB_enter (g_syms, S_Symbol("public"       ), (void *) (intptr_t) S_PUBLIC);
    TAB_enter (g_syms, S_Symbol("protected"    ), (void *) (intptr_t) S_PROTECTED);
    TAB_enter (g_syms, S_Symbol("internal"     ), (void *) (intptr_t) S_INTERNAL);
    TAB_enter (g_syms, S_Symbol("private"      ), (void *) (intptr_t) S_PRIVATE);
    TAB_enter (g_syms, S_Symbol("abstract"     ), (void *) (intptr_t) S_ABSTRACT);
    TAB_enter (g_syms, S_Symbol("sealed"       ), (void *) (intptr_t) S_SEALED);
    TAB_enter (g_syms, S_Symbol("static"       ), (void *) (intptr_t) S_STATIC);
    TAB_enter (g_syms, S_Symbol("readonly"     ), (void *) (intptr_t) S_READONLY);
    TAB_enter (g_syms, S_Symbol("unsafe"       ), (void *) (intptr_t) S_UNSAFE);
    TAB_enter (g_syms, S_Symbol("ref"          ), (void *) (intptr_t) S_REF);
    TAB_enter (g_syms, S_Symbol("partial"      ), (void *) (intptr_t) S_PARTIAL);
    TAB_enter (g_syms, S_Symbol("class"        ), (void *) (intptr_t) S_CLASS);
    TAB_enter (g_syms, S_Symbol("struct"       ), (void *) (intptr_t) S_STRUCT);
    TAB_enter (g_syms, S_Symbol("interface"    ), (void *) (intptr_t) S_INTERFACE);
    TAB_enter (g_syms, S_Symbol("enum"         ), (void *) (intptr_t) S_ENUM);
    TAB_enter (g_syms, S_Symbol("delegate"     ), (void *) (intptr_t) S_DELEGATE);
    TAB_enter (g_syms, S_Symbol("where"        ), (void *) (intptr_t) S_WHERE);
    TAB_enter (g_syms, S_Symbol("extern"       ), (void *) (intptr_t) S_EXTERN);
    TAB_enter (g_syms, S_Symbol("virtual"      ), (void *) (intptr_t) S_VIRTUAL);
    TAB_enter (g_syms, S_Symbol("override"     ), (void *) (intptr_t) S_OVERRIDE);
    TAB_enter (g_syms, S_Symbol("async"        ), (void *) (intptr_t) S_ASYNC);
    TAB_enter (g_syms, S_Symbol("void"         ), (void *) (intptr_t) S_VOID);
    TAB_enter (g_syms, S_Symbol("params"       ), (void *) (intptr_t) S_PARAMS);
    TAB_enter (g_syms, S_Symbol("if"           ), (void *) (intptr_t) S_IF       );
    TAB_enter (g_syms, S_Symbol("switch"       ), (void *) (intptr_t) S_SWITCH   );
    TAB_enter (g_syms, S_Symbol("while"        ), (void *) (intptr_t) S_WHILE    );
    TAB_enter (g_syms, S_Symbol("do"           ), (void *) (intptr_t) S_DO       );
    TAB_enter (g_syms, S_Symbol("for"          ), (void *) (intptr_t) S_FOR      );
    TAB_enter (g_syms, S_Symbol("foreach"      ), (void *) (intptr_t) S_FOREACH  );
    TAB_enter (g_syms, S_Symbol("var"          ), (void *) (intptr_t) S_VAR      );
    TAB_enter (g_syms, S_Symbol("await"        ), (void *) (intptr_t) S_AWAIT    );
    TAB_enter (g_syms, S_Symbol("as"           ), (void *) (intptr_t) S_AS);
    TAB_enter (g_syms, S_Symbol("is"           ), (void *) (intptr_t) S_IS);
}

