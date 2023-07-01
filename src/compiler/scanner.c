#define ENABLE_DEBUG

#include <ctype.h>
#include <stdint.h>
#include <math.h>

#include "scanner.h"
#include "util.h"
#include "errormsg.h"
#include "options.h"
#include "symbol.h"
#include "logger.h"

S_tkn_t S_tkn;

static const char   *g_sourcefn;
static FILE         *g_sourcef;

static char          g_ch;
static bool          g_eof = true;
static uint16_t      g_line, g_col;
static bool          g_eol = false;
static char          g_str[MAX_LINE_LEN];

static char          g_cur_line[MAX_LINE_LEN];

static TAB_table     g_src;     // line number -> string

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
        case S_IDENT    : LOG_printf(LOG_DEBUG, "[IDENT %s]",  S_name(S_tkn.u.sym)); break;
        case S_STRING   : LOG_printf(LOG_DEBUG, "[STRING %s]", S_tkn.u.str); break;
        case S_EOF      : LOG_printf(LOG_DEBUG, "[EOF]"); break;
        case S_PERIOD   : LOG_printf(LOG_DEBUG, "."); break;
        case S_LBRACE   : LOG_printf(LOG_DEBUG, "{"); break;
        case S_RBRACE   : LOG_printf(LOG_DEBUG, "}"); break;
        case S_SEMICOLON: LOG_printf(LOG_DEBUG, ";"); break;
        case S_COLON    : LOG_printf(LOG_DEBUG, ":"); break;
        case S_COMMA    : LOG_printf(LOG_DEBUG, ","); break;
        case S_ASTERISK : LOG_printf(LOG_DEBUG, "*"); break;
        case S_SLASH    : LOG_printf(LOG_DEBUG, "/"); break;
        case S_EQUALS   : LOG_printf(LOG_DEBUG, "="); break;
        case S_LESS     : LOG_printf(LOG_DEBUG, "<"); break;
        case S_LBRACKET : LOG_printf(LOG_DEBUG, "["); break;
        case S_RBRACKET : LOG_printf(LOG_DEBUG, "]"); break;
        case S_LPAREN   : LOG_printf(LOG_DEBUG, "("); break;
        case S_RPAREN   : LOG_printf(LOG_DEBUG, ")"); break;
    }
}
#else
static inline void _print_token(void)
{
}
#endif

//static void print_tkns(S_tkn tkn)
//{
//    printf("\n      TOKENS: ");
//    while (tkn)
//    {
//        print_tkn(tkn);
//        tkn = tkn->next;
//        if (tkn)
//            printf(" ");
//    }
//    printf("\n");
//}

static bool nextch (void)
{
    int n = fread(&g_ch, 1, 1, g_sourcef);
    return n==1;
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

#if 0
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
#endif

#if 0
static void number(int base, bool dp)
{
    assert(false);
    int    d;
    if (!tkn)
        tkn = S_Tkn(S_INUM);

    tkn->u.literal.inum = 0;

    if (!dp)
    {
        while (get_digit(&d, base))
        {
            tkn->u.literal.inum = d + tkn->u.literal.inum*base;
            getch();
        }
    }
    if (g_ch == '!')
    {
        getch();
        tkn->kind = S_FNUM;
        tkn->u.literal.fnum = tkn->u.literal.inum;
    }
    else
    {
        if (dp || (g_ch == '.'))
        {
            double m = 1.0 / base;
            tkn->kind = S_FNUM;
            tkn->u.literal.fnum = tkn->u.literal.inum;
            if (!dp)
                getch();
            while (get_digit(&d, base))
            {
                tkn->u.literal.fnum += ((double) d) * m;
                m /= base;
                getch();
            }
        }
        if ( (g_ch == 'e') || (g_ch == 'E') )
        {
            bool negative = false;
            if (tkn->kind == S_INUM)
            {
                tkn->kind = S_FNUM;
                tkn->u.literal.fnum = tkn->u.literal.inum;
            }
            getch();
            if (g_ch=='-')
            {
                negative = true;
                getch();
            }
            else
            {
               if (g_ch=='+')
                getch();
            }
            int e = 0;
            while (get_digit(&d, base))
            {
                e = d + e*base;
                getch();
            }
            if (negative)
                e = -1 * e;
            tkn->u.literal.fnum *= pow(base, e);
        }
    }

    // type suffix?
    switch (g_ch)
    {
        case '!':
        case 'F':
        case 'f':
            tkn->u.literal.typeHint = S_thSingle;
            getch();
            break;
        case '#':
            tkn->u.literal.typeHint = S_thDouble;
            getch();
            break;
        case 'L':
        case 'l':
        case '&':
            tkn->u.literal.typeHint = S_thLong;
            getch();
            break;
        case 'U':
        case 'u':
            tkn->u.literal.typeHint = S_thUInteger;
            getch();
            if ((g_ch == 'l') || (g_ch == 'L'))
            {
                tkn->u.literal.typeHint = S_thULong;
                getch();
            }
            break;
        default:
            tkn->u.literal.typeHint = S_thNone;
    }

    return tkn;
}
#endif

#if 0
static void handle_comment(bool line_comment)
{
    // printf ("skipping comment:\n");
    while (!g_eof && (g_ch != '\n'))
    {
        // printf ("%c", g_ch);
        getch();
    }
    // printf ("\ncomment done. eof: %d\n", g_eof);
}
#endif


static void identifier(void)
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
            identifier();
            goto done;
        }
        else
        {
            switch (g_ch)
            {
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
                    goto done;
                case '<':
                    getch();
                    S_tkn.kind = S_LESS;
                    goto done;
                case '=':
                    getch();
                    S_tkn.kind = S_EQUALS;
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

