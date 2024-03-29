#include <ctype.h>
#include <stdint.h>
#include <math.h>

#include "scanner.h"
#include "util.h"
#include "errormsg.h"
#include "options.h"
#include "symbol.h"

#define MAX_TOKENS      256
#define MAX_STRINGS      16
#define MAX_STRING_LEN 1024

static nextch_cb_t   g_cb = NULL;
static void         *g_user_data = NULL;

static char          g_ch;
static bool          g_eof = TRUE;
static int           g_line, g_col;
static bool          g_eol = FALSE;
static S_pos         g_pos = 0;

static struct S_tkn_ g_tkns[MAX_TOKENS];
static int           g_tkns_i=0;
static char          g_strings[MAX_STRINGS][MAX_STRING_LEN];
static int           g_strings_i=0;

static S_symbol      g_sym_rem;

static char          g_cur_line[MAX_LINE_LEN];
static int           g_cur_line_num;
static bool          g_filter_comments;

static bool          g_keep_source;
static U_poolId      g_pid;
static TAB_table     g_src;     // line number -> string

static void remember_pos(void)
{
    g_pos = (g_col << 16) | g_line;
}

int S_getcol(S_pos pos)
{
    return pos >> 16;
}
int S_getline(S_pos pos)
{
    return pos & 0xffff;
}

static S_tkn S_Tkn(int kind)
{
    if (g_tkns_i>=MAX_TOKENS)
    {
        assert(FALSE);
        return NULL;
    }

    S_tkn p = &g_tkns[g_tkns_i];
    g_tkns_i++;

    p->next = NULL;
    p->pos  = g_pos;
    p->kind = kind;

    return p;
}

static void print_tkn(S_tkn tkn)
{
    switch (tkn->kind)
    {
        case S_ERRTKN:     printf("[ERR]");       break;
        case S_EOL:        printf("[EOL]");       break;
        case S_LCOMMENT:   printf("[LCOMMENT %s]", tkn->u.str); break;
        case S_RCOMMENT:   printf("[RCOMMENT %s]", tkn->u.str); break;
        case S_IDENT:      printf("[IDENT %s]",  S_name(tkn->u.sym)); break;
        case S_STRING:     printf("[STRING %s]", tkn->u.str);         break;
        case S_COLON:      printf("[COLON]");     break;
        case S_SEMICOLON:  printf("[SEMICOLON]"); break;
        case S_COMMA:      printf("[COMMA]");     break;
        case S_INUM:       printf("[INUM %d]", tkn->u.literal.inum);  break;
        case S_FNUM:       printf("[FNUM %f]", tkn->u.literal.fnum);  break;
        case S_MINUS:      printf("[MINUS]");     break;
        case S_LPAREN:     printf("[LPAREN]");    break;
        case S_RPAREN:     printf("[RPAREN]");    break;
        case S_EQUALS:     printf("[EQUALS]");    break;
        case S_EXP:        printf("[EXP]");       break;
        case S_ASTERISK:   printf("[ASTERISK]");  break;
        case S_SLASH:      printf("[SLASH]");     break;
        case S_BACKSLASH:  printf("[BACKSLASH]"); break;
        case S_PLUS:       printf("[PLUS]");      break;
        case S_GREATER:    printf("[GREATER]");   break;
        case S_LESS:       printf("[LESS]");      break;
        case S_HASH:       printf("[HASH]");      break;
        case S_NOTEQ:      printf("[NOTEQ]");     break;
        case S_LESSEQ:     printf("[LESSEQ]");    break;
        case S_GREATEREQ:  printf("[GREATEREQ]"); break;
        case S_POINTER:    printf("[POINTER]");   break;
        case S_PERIOD:     printf("[PERIOD]");    break;
        case S_AT:         printf("[AT]");        break;
        case S_LBRACKET:   printf("[LBRACKET]");  break;
        case S_RBRACKET:   printf("[RBRACKET]");  break;
        case S_TRIPLEDOTS: printf("[TRIPLEDOTS]");break;
    }
}
static void print_tkns(S_tkn tkn)
{
    printf("\n      TOKENS: ");
    while (tkn)
    {
        print_tkn(tkn);
        tkn = tkn->next;
        if (tkn)
            printf(" ");
    }
    printf("\n");
}

static void getch(void)
{
    if (g_eol)
    {
        g_eol = FALSE;
        g_col = 1;
        if (g_keep_source)
            TAB_enter (g_src, (void *) (long) g_line, String(g_pid, g_cur_line));
        g_line++;
    }
    else
    {
        g_col++;
    }

    g_eof = !g_cb(&g_ch, g_user_data);
    if (!g_eof)
    {
        if (g_ch == '\n')
        {
            g_eol = TRUE;
        }
        else
        {
            if (g_col<MAX_LINE_LEN)
            {
                g_cur_line[g_col-1] = g_ch;
                g_cur_line[g_col]   = 0;
            }
            if (g_col==1)
                g_cur_line_num = g_line;
        }
    }
}

char *S_getcurline(void)
{
    return g_cur_line;
}

int S_getcurlinenum(void)
{
    return g_cur_line_num;
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
            return FALSE;
    if (d>=base)
        return FALSE;
    *digit = d;
    return TRUE;
}

static S_tkn number(int base, S_tkn tkn, bool dp)
{
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
            bool negative = FALSE;
            if (tkn->kind == S_INUM)
            {
                tkn->kind = S_FNUM;
                tkn->u.literal.fnum = tkn->u.literal.inum;
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

static S_tkn handle_comment(bool line_comment)
{
    if (g_filter_comments)
    {
        // printf ("skipping comment:\n");
        while (!g_eof && (g_ch != '\n'))
        {
            // printf ("%c", g_ch);
            getch();
        }
        // printf ("\ncomment done. eof: %d\n", g_eof);
        return NULL;
    }

    if (g_strings_i >= MAX_STRINGS)
        return NULL;

    char *str = g_strings[g_strings_i];
    int l = 0;
    S_tkn tkn = S_Tkn(line_comment ? S_LCOMMENT: S_RCOMMENT);
    while (!g_eof && (g_ch != '\n'))
    {
        str[l] = g_ch;
        l++;
        getch();
    }
    str[l] = '\0';
    tkn->u.str = str;

    return tkn;
}


static S_tkn ident(char ch)
{
    int l = 0;

    if (g_strings_i >= MAX_STRINGS)
        return NULL;
    char *str = g_strings[g_strings_i];

    S_tkn tkn = S_Tkn(S_IDENT);

    str[l] = ch;
    l++;
    while (S_isIDCont(g_ch) && !g_eof)
    {
        str[l] = g_ch;
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
            str[l] = g_ch;
            l++;
            getch();
            break;
    }
    str[l] = '\0';

    tkn->u.sym = S_Symbol(str);

    return tkn;
}

static S_tkn next_token(void)
{
    while (TRUE)
    {
        // skip whitespace, line continuations
        while ((S_isWhitespace(g_ch) || g_ch=='_') && !g_eof)
        {
            if (g_ch=='_')
            {
                getch();
                if (g_ch == '\r')
                    getch();
                if (g_ch == '\n')
                    getch();
                else
                    return ident('_');
            }
            else
            {
                getch();
            }
        }

        // handle line comments
        if (!g_eof && (g_ch == '\''))
        {
            getch();
            S_tkn tkn = handle_comment(/*line_comment=*/TRUE);
            if (!g_filter_comments)
                return tkn;
        }

        if (g_eof)
            return NULL;

        remember_pos();

        if (S_isIDStart(g_ch))
        {
            char ch = g_ch; getch();
            S_tkn tkn = ident(ch);
            if (tkn->u.sym == g_sym_rem)
            {
                S_tkn tkn = handle_comment(/*line_comment=*/FALSE);
                if (!g_filter_comments)
                    return tkn;
                if (g_eof)
                    return NULL;
                g_tkns_i--;
                continue;
            }
            else
            {
                return tkn;
            }
        }
        if (S_isDigit(g_ch))
        {
            return number (10, NULL, /*dp=*/FALSE);
        }

        S_tkn tkn;

        switch (g_ch)
        {
            case '"':
            {
                if (g_strings_i >= MAX_STRINGS)
                    return NULL;
                char *str = g_strings[g_strings_i];
                g_strings_i++;
                int l = 0;

                tkn = S_Tkn(S_STRING);
                tkn->u.str = str;
                getch();

                while ( (g_ch != '"') && !g_eof )
                {
                    str[l] = g_ch;
                    l++;
                    getch();
                }
                if (g_ch == '"')
                    getch();
                str[l] = '\0';
                break;
            }
            case '\n':
                tkn = S_Tkn(S_EOL);
                getch();
                break;
            case ':':
                tkn = S_Tkn(S_COLON);
                getch();
                break;
            case ';':
                tkn = S_Tkn(S_SEMICOLON);
                getch();
                break;
            case ',':
                tkn = S_Tkn(S_COMMA);
                getch();
                break;
            case '(':
                tkn = S_Tkn(S_LPAREN);
                getch();
                break;
            case ')':
                tkn = S_Tkn(S_RPAREN);
                getch();
                break;
            case '[':
                tkn = S_Tkn(S_LBRACKET);
                getch();
                break;
            case ']':
                tkn = S_Tkn(S_RBRACKET);
                getch();
                break;
            case '=':
                tkn = S_Tkn(S_EQUALS);
                getch();
                break;
            case '@':
                tkn = S_Tkn(S_AT);
                getch();
                break;
            case '-':
                tkn = S_Tkn(S_MINUS);
                getch();
                if (g_ch == '>')
                {
                    tkn->kind = S_POINTER;
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
                return number(10, NULL, /*dp=*/FALSE);

            case '&':   // binary, octal and hex literals
                tkn = S_Tkn(S_INUM);
                getch();
                switch (g_ch)
                {
                    case 'b':
                    case 'B':
                        getch();
                        number(2, tkn, /*dp=*/FALSE);
                        break;
                    case 'o':
                    case 'O':
                        getch();
                        number(8, tkn, /*dp=*/FALSE);
                        break;
                    case 'h':
                    case 'H':
                        getch();
                        number(16, tkn, /*dp=*/FALSE);
                        break;
                    default:
                        EM_error(tkn->pos, "lexer error: invalid literal type character");
                        getch();
                        number(10, tkn, /*dp=*/FALSE);
                        break;
                }
                break;
            case '^':
                tkn = S_Tkn(S_EXP);
                getch();
                break;
            case '*':
                tkn = S_Tkn(S_ASTERISK);
                getch();
                break;
            case '/':
                tkn = S_Tkn(S_SLASH);
                getch();
                break;
            case '\\':
                tkn = S_Tkn(S_BACKSLASH);
                getch();
                break;
            case '+':
                tkn = S_Tkn(S_PLUS);
                getch();
                break;
            case '.':
                tkn = S_Tkn(S_PERIOD);
                getch();
                if (S_isDigit(g_ch))
                    return number(10, NULL, /*dp=*/TRUE);
                if (g_ch == '.')
                {
                    getch();
                    if (g_ch == '.')
                    {
                        tkn = S_Tkn(S_TRIPLEDOTS);
                        getch();
                    }
                    else
                    {
                        tkn = S_Tkn(S_ERRTKN);
                    }
                }
                break;
            case '>':
                tkn = S_Tkn(S_GREATER);
                getch();
                if (g_ch == '=')
                {
                    tkn->kind = S_GREATEREQ;
                    getch();
                }
                break;
            case '<':
                tkn = S_Tkn(S_LESS);
                getch();
                if (g_ch == '=')
                {
                    tkn->kind = S_LESSEQ;
                    getch();
                }
                else
                {
                    if (g_ch == '>')
                    {
                        tkn->kind = S_NOTEQ;
                        getch();
                    }
                }
                break;
            case '#':
                tkn = S_Tkn(S_HASH);
                getch();
                break;
            default:
                tkn = S_Tkn(S_ERRTKN);
                getch();
        }

        return tkn;
    }
}


S_tkn S_nextline(void)
{
    if (g_eof)
        return NULL;

    S_tkn first_tkn=NULL, last_tkn=NULL;
    g_tkns_i    = 0;
    g_strings_i = 0;

    while (TRUE)
    {
        S_tkn tkn = next_token();

        if (!tkn)
        {
            if (!first_tkn)
                return NULL;
            tkn = S_Tkn(S_EOL);
        }
        else
        {
            if ((tkn->kind == S_EOL) && !first_tkn)
            {
                g_tkns_i    = 0;
                continue;
            }
        }

        if (last_tkn)
        {
            last_tkn->next = tkn;
            last_tkn = tkn;
        }
        else
        {
            first_tkn = last_tkn = tkn;
        }

        if ((tkn->kind == S_EOL) || (tkn->kind == S_COLON))
        {
            break;
        }
    }

    if (OPT_get(OPTION_VERBOSE))
    {
        if (g_keep_source)
            printf ("%5d %s", g_line-1, S_getSourceLine(g_line-1));
        print_tkns(first_tkn);
    }

    return first_tkn;
}

void S_init(U_poolId pid, bool keep_source, nextch_cb_t cb, void *user_data, bool filter_comments)
{
    g_pid             = pid;
    g_keep_source     = keep_source;
    if (g_keep_source)
        g_src         = TAB_empty(pid);
    g_sym_rem = S_Symbol("REM");

    g_cb              = cb;
    g_user_data       = user_data;
    g_eof             = FALSE;
    g_eol             = FALSE;
    g_line            = 1;
    g_col             = 0;

    g_cur_line[0]     = 0;
    g_cur_line_num    = 0;
    g_filter_comments = filter_comments;

    getch();
}

string  S_getSourceLine (int line)
{
    if (!line)
        return "";
    string s = TAB_look (g_src, (void*) (long) line);
    return s ? s : "";
}
