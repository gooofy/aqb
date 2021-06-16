#include "ide.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <libgen.h>

#include "util.h"
#include "ui.h"
#include "scanner.h"
#include "frontend.h"
#include "compiler.h"
#include "logger.h"
#include "run.h"
#include "options.h"
#include "errormsg.h"

//#define LOG_SLOWDOWN

//#define ENABLE_REPAINT_BENCHMARK

#define INFOLINE            "F1:help X=   1 Y=   1 #=   0  new file"
#define INFOLINE_CURSOR_X     10
#define INFOLINE_CURSOR_Y     17
#define INFOLINE_NUM_LINES    24
#define INFOLINE_CHANGED      29
#define INFOLINE_FILENAME     30

#define CR  13
#define LF  10
#define TAB  9

#define SCROLL_MARGIN 5

#define INDENT_SPACES 4

#ifdef __amigaos__
#define TMP_BINFN "T:aqb_out.exe"
#else
#define TMP_BINFN "/tmp/aqb_out.exe"
#endif

static S_symbol S_IF;
static S_symbol S_ELSEIF;
static S_symbol S_ELSE;
static S_symbol S_THEN;
static S_symbol S_END;
static S_symbol S_SUB;
static S_symbol S_FUNCTION;
static S_symbol S_FOR;
static S_symbol S_NEXT;
static S_symbol S_DO;
static S_symbol S_LOOP;
static S_symbol S_WHILE;
static S_symbol S_WEND;
static S_symbol S_SELECT;
static S_symbol S_CASE;

typedef struct IDE_line_     *IDE_line;
typedef struct IDE_editor_   *IDE_editor;

struct IDE_line_
{
	IDE_line  next, prev;
	uint16_t  len;
    char     *buf;
    char     *style;

    int8_t    indent;
    int8_t    pre_indent, post_indent;

    // folding support
    bool      folded, fold_start, fold_end;
};

struct IDE_editor_
{
    IDE_line           line_first, line_last;
	uint16_t           num_lines;
	bool               changed;
	char               infoline[36+PATH_MAX];
    uint16_t           infoline_row;
	char 	           sourcefn[PATH_MAX];
	char 	           module_name[PATH_MAX];
	char 	           binfn[PATH_MAX];

	int16_t            cursor_col, cursor_row;
    IDE_line           cursor_line;

    // window, scrolling, repaint
	int16_t            window_width, window_height;
	int16_t            scrolloff_col, scrolloff_row;
    IDE_line           scrolloff_line;
    int16_t            scrolloff_line_row;
    bool               up2date_row[UI_MAX_ROWS];
    bool               up2date_il_pos;
    bool               up2date_il_num_lines;
    bool               up2date_il_flags;
    bool               up2date_il_sourcefn;
    bool               il_show_error;
    bool               repaint_all;

    // cursor_line currently being edited (i.e. represented in buffer):
    bool               editing;
    char               buf[MAX_LINE_LEN];
    char               style[MAX_LINE_LEN];
    uint16_t           buf_len;
    uint16_t           buf_pos;

    // temporary buffer used in edit operations
    char               buf2[MAX_LINE_LEN];
    char               style2[MAX_LINE_LEN];
    uint16_t           buf2_len;
};

#if LOG_LEVEL == LOG_DEBUG
static FILE *logf=NULL;
#endif
static IDE_editor g_ed;
static TAB_table  g_keywords;

IDE_line newLine(IDE_editor ed, char *buf, char *style, int8_t pre_indent, int8_t post_indent, bool fold_start, bool fold_end)
{
    int len = strlen(buf);
    IDE_line l = U_poolAlloc (UP_ide, sizeof(*l)+2*(len+1));

	l->next        = NULL;
	l->prev        = NULL;
	l->len         = len;
    l->buf         = (char *) (&l[1]);
    l->style       = l->buf + len + 1;
    l->indent      = 0;
    l->pre_indent  = pre_indent;
    l->post_indent = post_indent;
    l->folded      = FALSE;
    l->fold_start  = fold_start;
    l->fold_end    = fold_end;

    memcpy (l->buf, buf, len+1);
    memcpy (l->style, style, len+1);

	return l;
}

static void freeLine (IDE_editor ed, IDE_line l)
{
    // FIXME: implement
    ed->scrolloff_line = NULL;
}

static void insertLineAfter (IDE_editor ed, IDE_line lBefore, IDE_line l)
{
    if (lBefore)
    {
        if (lBefore == ed->line_last)
        {
            ed->line_last = l;
            l->next = NULL;
        }
        else
        {
            l->next = lBefore->next;
            lBefore->next->prev = l;
        }
        l->prev = lBefore;
        lBefore->next = l;
    }
    else
    {
        l->next = ed->line_first;
        if (ed->line_first)
        {
            ed->line_first = ed->line_first->prev = l;
        }
        else
        {
            ed->line_first = ed->line_last = l;
        }
    }
}

static void deleteLine (IDE_editor ed, IDE_line l)
{
    if (l->prev)
        l->prev->next = l->next;
    else
        ed->line_first = l->next;
    if (l->next)
        l->next->prev = l->prev;
    else
        ed->line_last = l->prev;
    freeLine (ed, l);
}

void initWindowSize (IDE_editor ed)
{
    uint16_t rows, cols;
    if (!UI_getsize (&rows, &cols))
        exit(1);

    ed->window_width  = cols;
    ed->window_height = rows;
    ed->infoline_row  = rows-1;
    UI_setScrollArea (1, rows-1);
}

static void _itoa(uint16_t num, char* buf, uint16_t width)
{
    uint16_t i = 0;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        for (uint16_t j=1; j<width; j++)
            buf[i++] = ' ';
        buf[i++] = '0';
        return;
    }

    // Process individual digits
    uint16_t b=1;
    for (uint16_t j=1; j<width; j++)
        b = b * 10;

    for (uint16_t j=0; j<width; j++)
    {
        uint16_t digit = (num / b) % 10;
        b = b / 10;
        buf[i++] = digit + '0';
    }
}

static IDE_line getLine (IDE_editor ed, int linenum)
{
    IDE_line l = ed->line_first;
    int ln = 0;
    while ( l && (ln < linenum) )
    {
        ln++;
        l = l->next;
    }
    return l;
}

static void invalidateAll (IDE_editor ed)
{
    for (uint16_t i=0; i<UI_MAX_ROWS; i++)
        ed->up2date_row[i] = FALSE;
    ed->up2date_il_pos       = FALSE;
    ed->up2date_il_num_lines = FALSE;
    ed->up2date_il_flags     = FALSE;
    ed->up2date_il_sourcefn  = FALSE;
    ed->repaint_all          = TRUE;
}

static void scroll(IDE_editor ed)
{
    int16_t cursor_row = ed->cursor_row - ed->scrolloff_row;

    // scroll up ?

    if (cursor_row > ed->window_height - SCROLL_MARGIN)
    {
        int16_t scrolloff_row_new = ed->cursor_row - ed->window_height + SCROLL_MARGIN;
        int16_t scrolloff_row_max = ed->num_lines - ed->window_height + 1;

        if (scrolloff_row_new > scrolloff_row_max)
            scrolloff_row_new = scrolloff_row_max;

        int16_t diff = scrolloff_row_new - ed->scrolloff_row;
        ed->scrolloff_row = scrolloff_row_new;
        switch (diff)
        {
            case 0:
                break;
            case 1:
                UI_scrollUp(/*fullscreen=*/FALSE);
                ed->up2date_row[ed->window_height - 2] = FALSE;
                break;
            default:
                invalidateAll (ed);
        }
    }

    // scroll down ?

    if (cursor_row < SCROLL_MARGIN)
    {
        int16_t scrolloff_row_new = ed->cursor_row > SCROLL_MARGIN ? ed->cursor_row - SCROLL_MARGIN : 0;

        int16_t diff = ed->scrolloff_row - scrolloff_row_new;
        ed->scrolloff_row = scrolloff_row_new;
        switch (diff)
        {
            case 0:
                break;
            case 1:
                UI_scrollDown();
                ed->up2date_row[0] = FALSE;
                break;
            default:
                invalidateAll (ed);
        }
    }
    // FIXME: implement horizontal scroll
}

static bool nextch_cb(char *ch, void *user_data)
{
    IDE_editor ed = user_data;
    *ch = ed->buf[ed->buf_pos++];
    return (*ch) != 0;
}

typedef enum { STAUI_IDLE, STAUI_IF, STAUI_ELSEIF, STAUI_ELSE, STAUI_THEN,
               STAUI_ELSEIFTHEN, STAUI_LOOP, STAUI_LOOPEND,
               STAUI_END, STAUI_SUB, STAUI_SELECT, STAUI_CASE } state_enum;


static IDE_line buf2line (IDE_editor ed)
{
    static char buf[MAX_LINE_LEN];
    static char style[MAX_LINE_LEN];

    ed->buf_pos = 0;
    ed->buf[ed->buf_len]=0;
    S_init (nextch_cb, ed, /*filter_comments=*/FALSE);

    int pos = 0;
    int8_t pre_indent = 0;
    int8_t post_indent = 0;
    bool fold_start = FALSE;
    bool fold_end   = FALSE;
    while (TRUE)
    {
        S_tkn tkn = S_nextline();
        if (!tkn)
            break;
        bool first = TRUE;
        S_token lastKind = S_ERRTKN;
        S_tkn lastTkn = NULL;
        state_enum state = STAUI_IDLE;
        while (tkn && (pos <MAX_LINE_LEN-1))
        {
            switch (tkn->kind)
            {
                case S_ERRTKN:
                    break;
                case S_COLON:
                    buf[pos] = ' ';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    buf[pos] = ':';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    buf[pos] = ' ';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    /* fall through */
                case S_EOL:
                    switch (state)
                    {
                        case STAUI_THEN:
                            if ((lastKind == S_IDENT) && (lastTkn->u.sym == S_THEN))
                            {
                                post_indent++;
                            }
                            break;
                        case STAUI_ELSEIFTHEN:
                            if ((lastKind == S_IDENT) && (lastTkn->u.sym == S_THEN))
                            {
                                pre_indent--;
                                post_indent++;
                            }
                            break;
                        case STAUI_ELSE:
                            if ((lastKind == S_IDENT) && (lastTkn->u.sym == S_ELSE))
                            {
                                pre_indent--;
                                post_indent++;
                            }
                            break;
                        case STAUI_SUB:
                        case STAUI_SELECT:
                            post_indent++;
                            break;
                        case STAUI_CASE:
                            pre_indent--;
                            post_indent++;
                            break;
                        case STAUI_END:
                            pre_indent--;
                            break;
                        case STAUI_LOOP:
                        case STAUI_LOOPEND:
                            break;
                        default:
                            break;
                    }
                    break;
                case S_LCOMMENT:
                    buf[pos] = '\'';
                    style[pos++] = UI_TEXT_STYLE_COMMENT;
                    for (char *c=tkn->u.str; *c; c++)
                    {
                        buf[pos] = *c;
                        style[pos++] = UI_TEXT_STYLE_COMMENT;
                    }
                    break;
                case S_RCOMMENT:
                    buf[pos] = 'R';
                    style[pos++] = UI_TEXT_STYLE_COMMENT;
                    buf[pos] = 'E';
                    style[pos++] = UI_TEXT_STYLE_COMMENT;
                    buf[pos] = 'M';
                    style[pos++] = UI_TEXT_STYLE_COMMENT;
                    for (char *c=tkn->u.str; *c; c++)
                    {
                        buf[pos] = *c;
                        style[pos++] = UI_TEXT_STYLE_COMMENT;
                    }
                    break;
                case S_IDENT:
                {
                    if (!first)
                        buf[pos++] = ' ';
                    bool is_kw = TAB_look(g_keywords, tkn->u.sym) != NULL;
                    char *s = S_name(tkn->u.sym);
                    int l = strlen(s);
                    if (pos+l >= MAX_LINE_LEN-1)
                        l = MAX_LINE_LEN-1-pos;
                    for (int i =0; i<l; i++)
                    {
                        if (is_kw)
                        {
                            buf[pos] = toupper(s[i]);
                            style[pos++] = UI_TEXT_STYLE_KEYWORD;
                        }
                        else
                        {
                            buf[pos] = s[i];
                            style[pos++] = UI_TEXT_STYLE_TEXT;
                        }
                    }

                    // auto-indentation is based on very crude BASIC parsing

                    switch (state)
                    {
                        case STAUI_IDLE:
                            if (tkn->u.sym == S_IF)
                            {
                                state = STAUI_IF;
                            }
                            else if (tkn->u.sym == S_ELSEIF)
                            {
                                state = STAUI_ELSEIF;
                            }
                            else if (tkn->u.sym == S_ELSE)
                            {
                                state = STAUI_ELSE;
                            }
                            else if (tkn->u.sym == S_END)
                            {
                                state = STAUI_END;
                            }
                            else if (tkn->u.sym == S_SUB)
                            {
                                state = STAUI_SUB;
                                fold_start = TRUE;
                            }
                            else if (tkn->u.sym == S_FUNCTION)
                            {
                                state = STAUI_SUB;
                                fold_start = TRUE;
                            }
                            else if (tkn->u.sym == S_FOR)
                            {
                                state = STAUI_LOOP;
                                post_indent++;
                            }
                            else if (tkn->u.sym == S_DO)
                            {
                                state = STAUI_LOOP;
                                post_indent++;
                            }
                            else if (tkn->u.sym == S_WHILE)
                            {
                                state = STAUI_LOOP;
                                post_indent++;
                            }
                            else if ((tkn->u.sym == S_NEXT) || (tkn->u.sym == S_LOOP) || (tkn->u.sym == S_WEND))
                            {
                                state = STAUI_LOOPEND;
                                if (post_indent>0)
                                    post_indent--;
                                else
                                    pre_indent--;
                            }
                            else if (tkn->u.sym == S_SELECT)
                            {
                                state = STAUI_SELECT;
                            }
                            else if (tkn->u.sym == S_CASE)
                            {
                                state = STAUI_CASE;
                            }
                            break;
                        case STAUI_IF:
                            if (tkn->u.sym == S_THEN)
                            {
                                state = STAUI_THEN;
                            }
                            break;
                        case STAUI_ELSEIF:
                            if (tkn->u.sym == S_THEN)
                            {
                                state = STAUI_ELSEIFTHEN;
                            }
                            break;
                        case STAUI_END:
                            if (tkn->u.sym == S_SUB)
                                fold_end = TRUE;
                            break;
                        case STAUI_THEN:
                        case STAUI_ELSEIFTHEN:
                        case STAUI_ELSE:
                        case STAUI_SUB:
                        case STAUI_LOOP:
                        case STAUI_LOOPEND:
                        case STAUI_SELECT:
                        case STAUI_CASE:
                            break;
                        default:
                            assert(FALSE);
                    }
                    break;
                }
                case S_STRING:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '"';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    for (char *c=tkn->u.str; *c; c++)
                    {
                        buf[pos] = *c;
                        style[pos++] = UI_TEXT_STYLE_TEXT;
                    }
                    buf[pos] = '"';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_SEMICOLON:
                    buf[pos] = ';';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_COMMA:
                    buf[pos] = ',';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_INUM:
                {
                    if (!first && (lastKind != S_MINUS))
                        buf[pos++] = ' ';
                    static char nbuf[64];
                    snprintf (nbuf, 64, "%d", tkn->u.literal.inum);
                    for (char *c=nbuf; *c; c++)
                    {
                        buf[pos] = *c;
                        style[pos++] = UI_TEXT_STYLE_TEXT;
                    }
                    break;
                }
                case S_FNUM:
                {
                    if (!first && (lastKind != S_MINUS))
                        buf[pos++] = ' ';
                    static char nbuf[64];
                    U_float2str (tkn->u.literal.fnum, nbuf, 64);
                    for (char *c=nbuf; *c; c++)
                    {
                        buf[pos] = *c;
                        style[pos++] = UI_TEXT_STYLE_TEXT;
                    }
                    break;
                }
                case S_MINUS:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '-';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_LPAREN:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '(';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_RPAREN:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = ')';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_EQUALS:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '=';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_EXP:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '^';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_ASTERISK:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '*';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_SLASH:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '/';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_BACKSLASH:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '\\';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_PLUS:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '+';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_GREATER:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '>';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_LESS:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '<';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_NOTEQ:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '<';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    buf[pos] = '>';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_LESSEQ:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '<';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    buf[pos] = '=';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_GREATEREQ:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '>';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    buf[pos] = '=';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_POINTER:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '-';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    buf[pos] = '>';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_PERIOD:
                    buf[pos] = '.';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_AT:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '@';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_LBRACKET:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '[';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_RBRACKET:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = ']';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
                case S_TRIPLEDOTS:
                    if (!first)
                        buf[pos++] = ' ';
                    buf[pos] = '.';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    buf[pos] = '.';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    buf[pos] = '.';
                    style[pos++] = UI_TEXT_STYLE_TEXT;
                    break;
            }

            lastKind = tkn->kind;
            lastTkn = tkn;
            tkn = tkn->next;
            first = FALSE;
        }
    }
    buf[pos] = 0;

    return newLine (ed, buf, style, pre_indent, post_indent, fold_start, fold_end);
}

static void indentLine (IDE_line l)
{
    if (!l->prev)
    {
        l->indent = 0;
        return;
    }
    l->indent = l->prev->indent + l->prev->post_indent + l->pre_indent;
    if (l->indent < 0)
        l->indent = 0;
    LOG_printf (LOG_DEBUG, "identLine: l->prev->indent (%d) + l->prev->post_indent (%d) + l->pre_indent (%d) = %d\n",
                l->prev->indent, l->prev->post_indent, l->pre_indent, l->indent);
}

static void indentSuccLines (IDE_editor ed, IDE_line lp)
{
    IDE_line l = lp->next;
    while (l)
    {
        indentLine (l);
        l = l->next;
    }
    invalidateAll (ed);
}

static IDE_line commitBuf(IDE_editor ed)
{
    IDE_line cl = ed->cursor_line;
    IDE_line l  = buf2line (ed);
    if (cl->prev)
        cl->prev->next = l;
    else
        ed->line_first = l;
    if (cl->next)
        cl->next->prev = l;
    else
        ed->line_last = l;
    l->next = cl->next;
    l->prev = cl->prev;
    ed->editing = FALSE;
    int8_t old_indent = cl->indent;
    int8_t old_post_indent = cl->post_indent;
    freeLine (ed, cl);
    indentLine (l);
    if ( (l->indent == old_indent) && (l->post_indent == old_post_indent) )
        ed->up2date_row[ed->cursor_row - ed->scrolloff_row] = FALSE;
    else
        indentSuccLines (ed, l);
    return l;
}

static bool cursorUp(IDE_editor ed)
{
    IDE_line pl = ed->cursor_line->prev;
    if (!pl)
        return FALSE;

    if (ed->editing)
        commitBuf (ed);

    ed->cursor_line = pl;
    ed->cursor_row--;
    if (ed->cursor_col > pl->len)
    {
        ed->cursor_col = pl->len;
    }
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool cursorDown(IDE_editor ed)
{
    IDE_line nl = ed->cursor_line->next;
    if (!nl)
        return FALSE;

    if (ed->editing)
        commitBuf (ed);

    ed->cursor_line = nl;
    ed->cursor_row++;
    if (ed->cursor_col > nl->len)
        ed->cursor_col = nl->len;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool cursorLeft(IDE_editor ed)
{
    if (ed->cursor_col == 0)
        return FALSE;

    ed->cursor_col--;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool cursorRight(IDE_editor ed)
{
    int len = ed->editing ? ed->buf_len : ed->cursor_line->len + INDENT_SPACES * ed->cursor_line->indent;
    if (ed->cursor_col >= len)
        return FALSE;

    ed->cursor_col++;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool pageUp(IDE_editor ed)
{
    for (uint16_t i = 0; i<ed->window_height; i++)
    {
        if (!cursorUp(ed))
            return FALSE;
    }
    return TRUE;
}

static bool pageDown(IDE_editor ed)
{
    for (uint16_t i = 0; i<ed->window_height; i++)
    {
        if (!cursorDown(ed))
            return FALSE;
    }
    return TRUE;
}

static bool gotoBOF(IDE_editor ed)
{
    if (ed->editing)
        commitBuf (ed);

    ed->cursor_line = ed->line_first;

    ed->cursor_row = 0;
    ed->cursor_col = 0;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool gotoEOF(IDE_editor ed)
{
    if (ed->editing)
        commitBuf (ed);

    ed->cursor_line = ed->line_first;
    ed->cursor_row = 0;
	while (ed->cursor_line->next)
	{
		ed->cursor_line = ed->cursor_line->next;
		ed->cursor_row++;
	}

    ed->cursor_col = 0;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool gotoHome(IDE_editor ed)
{
    ed->cursor_col = 0;
    ed->up2date_il_pos = FALSE;
    return TRUE;
}

static bool gotoEnd(IDE_editor ed)
{
    if (ed->editing)
        ed->cursor_col = ed->buf_len;
    else
        ed->cursor_col = ed->cursor_line->len + ed->cursor_line->indent * INDENT_SPACES;
    ed->up2date_il_pos = FALSE;
    return TRUE;
}

static bool gotoLine(IDE_editor ed, uint16_t line, uint16_t col)
{
    if (ed->editing)
        commitBuf (ed);

    ed->cursor_line = ed->line_first;

    for (uint16_t i = 1; i<line; i++)
    {
        if (!ed->cursor_line->next)
            break;
        ed->cursor_line = ed->cursor_line->next;
    }

    ed->cursor_row = line-1;
    ed->cursor_col = col-1;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static void line2buf (IDE_editor ed, IDE_line l)
{
    uint16_t off = 0;
    for (int8_t i = 0; i<l->indent; i++)
    {
        for (int8_t j = 0; j<INDENT_SPACES; j++)
        {
            ed->buf[off] = ' ';
            ed->style[off++] = UI_TEXT_STYLE_TEXT;
        }
    }

    memcpy (ed->buf+off,   l->buf  , l->len+1);
    memcpy (ed->style+off, l->style, l->len+1);
    ed->editing  = TRUE;
    if (!ed->changed)
    {
        ed->changed  = TRUE;
        ed->up2date_il_flags = FALSE;
    }
    ed->buf_len  = l->len+off;
}

static void repaintLine (IDE_editor ed, char *buf, char *style, uint16_t len, uint16_t row, uint16_t indent, bool folded)
{
    // FIXME: horizontal scroll

    UI_beginLine (row);

    char s=UI_TEXT_STYLE_TEXT;
    UI_setTextStyle (UI_TEXT_STYLE_TEXT);

    if (folded)
        UI_putc ('>');

    for (uint16_t i=0; i<indent; i++)
    {
        UI_putstr ("    ");
    }

    for (uint16_t i=0; i<len; i++)
    {
        char s2 = style[i];
        if (s2 != s)
		{
			UI_setTextStyle (s2);
            s = s2;
        }
        UI_putc (buf[i]);
    }
    UI_endLine();
}

static void repaint (IDE_editor ed)
{
#ifdef ENABLE_REPAINT_BENCHMARK
    float startTime = U_getTime();
#endif

    UI_setCursorVisible (FALSE);

    // cache first visible line for speed
    if (!ed->scrolloff_line || (ed->scrolloff_line_row != ed->scrolloff_row))
    {
        ed->scrolloff_line = getLine (ed, ed->scrolloff_row);
        ed->scrolloff_line_row = ed->scrolloff_row;
    }

    IDE_line l = ed->scrolloff_line;

    uint16_t linenum_end = ed->scrolloff_row + ed->window_height - 2;
    uint16_t linenum = ed->scrolloff_row;
    uint16_t row = 0;
    while (l && (linenum <= linenum_end))
    {
        if (!ed->up2date_row[row])
        {
            if (ed->editing && (linenum == ed->cursor_row))
                repaintLine (ed, ed->buf, ed->style, ed->buf_len, row + 1, 0, /*folded=*/FALSE);
            else
                repaintLine (ed, l->buf, l->style, l->len, row + 1, l->indent, l->folded);
            ed->up2date_row[row] = TRUE;
        }
        if (l->folded)
        {
            while (l && !l->fold_end)
            {
                l = l->next;
            }
            if (l)
                l = l->next;
        }
        else
        {
            l = l->next;
        }
        linenum++;
        row++;
    }

    if (ed->repaint_all)
    {
        ed->repaint_all = FALSE;
        while (row < ed->infoline_row)
        {
            UI_beginLine (row);
            UI_endLine   ();
            row++;
        }
    }

    // infoline

    bool update_infoline = FALSE;
    if (!ed->up2date_il_pos)
    {
        _itoa (ed->cursor_col+1, ed->infoline + INFOLINE_CURSOR_X, 4);
        _itoa (ed->cursor_row+1, ed->infoline + INFOLINE_CURSOR_Y, 4);
        update_infoline = TRUE;
        ed->up2date_il_pos = TRUE;
    }
    if (!ed->up2date_il_num_lines)
    {
        _itoa (ed->num_lines, ed->infoline + INFOLINE_NUM_LINES, 4);
        update_infoline = TRUE;
        ed->up2date_il_num_lines = TRUE;
    }
    if (!ed->up2date_il_flags)
    {
        char *fs = ed->infoline + INFOLINE_CHANGED;
        if (ed->changed)
            *fs = '*';
        else
            *fs = ' ';
        update_infoline = TRUE;
        ed->up2date_il_flags = TRUE;
    }
    if (!ed->up2date_il_sourcefn)
    {
        char *fs = ed->infoline + INFOLINE_FILENAME;
        if (!ed->il_show_error)
        {
            for (char *c = ed->sourcefn; *c; c++)
            {
                *fs++ = *c;
            }
            *fs = 0;
            update_infoline = TRUE;
            ed->up2date_il_sourcefn = TRUE;
        }
        else
        {
            for (char *c = EM_firstError; *c; c++)
            {
                *fs++ = *c;
            }
            *fs = 0;
            update_infoline = TRUE;
            ed->il_show_error = FALSE;
        }
    }

    if (update_infoline)
    {
        LOG_printf (LOG_DEBUG, "outputInfoLine: row=%d, txt=%s\n", ed->infoline_row, ed->infoline);
        UI_setTextStyle (UI_TEXT_STYLE_INVERSE);
        UI_beginLine (ed->infoline_row+1);
        char *c = ed->infoline;
        int col = 0;
        while (*c && col < ed->window_width)
        {
            UI_putc (*c++);
            col++;
        }
        while (col < ed->window_width)
        {
            UI_putc (' ');
            col++;
        }
        UI_setTextStyle (UI_TEXT_STYLE_TEXT);
        UI_endLine ();
    }

    UI_moveCursor (ed->cursor_row-ed->scrolloff_row+1, ed->cursor_col-ed->scrolloff_col+1);
    UI_setCursorVisible (TRUE);

#ifdef ENABLE_REPAINT_BENCHMARK
    float stopTime = U_getTime();
    printf ("IDE: repaint() took %d-%d = %d ms\n", (int)stopTime, (int)startTime, (int) (1000.0 * (stopTime-startTime)));
#endif
}

static void enterKey (IDE_editor ed)
{
    // split line ?
    uint16_t l = ed->editing ? ed->buf_len : ed->cursor_line->len + ed->cursor_line->indent*INDENT_SPACES;
    if (ed->cursor_col < l)
    {
        if (!ed->editing)
            line2buf (ed, ed->cursor_line);

        memcpy (ed->buf2,   ed->buf+ed->cursor_col  , l - ed->cursor_col);
        memcpy (ed->style2, ed->style+ed->cursor_col, l - ed->cursor_col);

        ed->buf_len = ed->cursor_col;
        ed->cursor_line = commitBuf (ed);

        l -= ed->cursor_col;

        memcpy (ed->buf,   ed->buf2  , l);
        memcpy (ed->style, ed->style2, l);
        ed->buf_len = l;
    }
    else
    {
        if (ed->editing)
            ed->cursor_line = commitBuf (ed);
        ed->buf_len = 0;
    }

    IDE_line line = buf2line (ed);
    insertLineAfter (ed, ed->cursor_line, line);
    indentSuccLines (ed, ed->cursor_line);
    ed->cursor_col = 0;
    ed->cursor_row++;
    ed->cursor_line = line;
    ed->num_lines++;
    invalidateAll(ed);
}

static void backspaceKey (IDE_editor ed)
{
    // join lines ?
    if (ed->cursor_col == 0)
    {
        if (!ed->cursor_line->prev)
            return;
        IDE_line cl = ed->cursor_line;
        if (ed->editing)
            cl = commitBuf (ed);
        IDE_line pl = cl->prev;
        line2buf (ed, pl);
        ed->cursor_col = ed->buf_len;
        ed->cursor_row--;
        ed->cursor_line = pl;
        ed->buf[ed->buf_len] = ' ';
        ed->style[ed->buf_len] = UI_TEXT_STYLE_TEXT;
        ed->buf_len++;
        memcpy (ed->buf+ed->buf_len,   cl->buf  , cl->len);
        memcpy (ed->style+ed->buf_len, cl->style, cl->len);
        ed->buf_len += cl->len;
        ed->editing = TRUE;
        deleteLine (ed, cl);
        ed->cursor_line = commitBuf (ed);
        invalidateAll(ed);
        ed->num_lines--;
    }
    else
    {
        if (!ed->editing)
            line2buf (ed, ed->cursor_line);

        for (uint16_t i=ed->cursor_col; i<ed->buf_len; i++)
        {
            ed->buf[i-1]   = ed->buf[i];
            ed->style[i-1] = ed->style[i];
        }
        ed->buf_len--;
        ed->up2date_row[ed->cursor_row - ed->scrolloff_row] = FALSE;
        cursorLeft(ed);
    }
}

static void deleteKey (IDE_editor ed)
{
    // join lines ?
    uint16_t l = ed->editing ? ed->buf_len : ed->cursor_line->len + ed->cursor_line->indent*INDENT_SPACES;
    if (ed->cursor_col == l)
    {
        if (!ed->cursor_line->next)
            return;
        IDE_line cl = ed->cursor_line;
        if (!ed->editing)
            line2buf (ed, ed->cursor_line);

        IDE_line nl = cl->next;
        memcpy (ed->buf+ed->buf_len,   nl->buf  , nl->len);
        memcpy (ed->style+ed->buf_len, nl->style, nl->len);
        ed->buf_len += nl->len;
        deleteLine (ed, nl);
        ed->cursor_line = commitBuf (ed);
        ed->num_lines--;
        invalidateAll(ed);
    }
    else
    {
        if (!ed->editing)
            line2buf (ed, ed->cursor_line);

        for (uint16_t i=ed->cursor_col; i<ed->buf_len-1; i++)
        {
            ed->buf[i]   = ed->buf[i+1];
            ed->style[i] = ed->style[i+1];
        }
        ed->buf_len--;
        ed->up2date_row[ed->cursor_row - ed->scrolloff_row] = FALSE;
    }
}


static void killLine (IDE_editor ed)
{
    IDE_line cl = ed->cursor_line;
    IDE_line nl = cl->next;

    if (!nl)
    {
        UI_bell();
        return;
    }

    if (ed->editing)
        cl = commitBuf (ed);

    if (ed->cursor_col > nl->len)
        ed->cursor_col = nl->len;

    ed->cursor_line = nl;
    deleteLine (ed, cl);
    invalidateAll(ed);
    ed->num_lines--;
}

static bool printableAsciiChar (uint16_t c)
{
    return (c >= 32) && (c <= 126);
}

// FIXME: looks like GCC will emit un-aligned word access code in our
//        copy loop
#ifdef __amigaos__
#pragma GCC push_options
#pragma GCC optimize ("O0")
#endif
static bool insertChar (IDE_editor ed, uint16_t c)
{
    LOG_printf (LOG_DEBUG, "insertChar %c\n", c);
    if (!printableAsciiChar(c))
    {
        LOG_printf (LOG_DEBUG, "ide.c: insertChar: non-printable char %d detected.\n", c);
        return FALSE;
    }

    if (!ed->editing)
    {
        line2buf (ed, ed->cursor_line);
    }
    //LOG_printf (LOG_DEBUG, "insertChar %c 2\n", c);

    uint16_t cp = ed->scrolloff_col + ed->cursor_col;
    //LOG_printf (LOG_DEBUG, "insertChar 2.1 cp=%d\n", cp);

    for (int i=ed->buf_len; i>cp; i--)
    {
        //LOG_printf (LOG_DEBUG, "insertChar 2.1 i=%d\n", i);
        ed->buf[i]   = ed->buf[i-1];
        ed->style[i] = ed->style[i-1];
    }
    //LOG_printf (LOG_DEBUG, "insertChar 2.2\n");
    ed->buf[cp]   = c;
    ed->style[cp] = UI_TEXT_STYLE_TEXT;
    ed->buf_len++;
    //LOG_printf (LOG_DEBUG, "insertChar %c 3\n", c);

    ed->up2date_row[ed->cursor_row - ed->scrolloff_row] = FALSE;

    cursorRight(ed);
    //LOG_printf (LOG_DEBUG, "insertChar %c 4\n", c);

    return TRUE;
}
#ifdef __amigaos__
#pragma GCC pop_options
#endif
static void IDE_save (IDE_editor ed)
{
    if (ed->editing)
        commitBuf (ed);

    FILE *sourcef = fopen(ed->sourcefn, "w");
    if (!sourcef)
    {
        fprintf(stderr, "failed to write %s: %s\n\n", ed->sourcefn, strerror(errno));
        exit(2);
    }

    static char *indent_str = "    ";
    static char *lf = "\n";

    for (IDE_line l = ed->line_first; l; l=l->next)
    {
        for (int8_t i = 0; i<l->indent; i++)
            fwrite (indent_str, 4, 1, sourcef);
        fwrite (l->buf, l->len, 1, sourcef);
        fwrite (lf, 1, 1, sourcef);
    }

    fclose (sourcef);

    ed->changed = FALSE;
    ed->up2date_il_flags = FALSE;
}

static void IDE_exit (IDE_editor ed)
{
    LOG_printf (LOG_DEBUG, "ide: IDE_exit\n");
    if (ed->changed)
    {
        if (UI_EZRequest ("Save changes to disk?", "Yes|No"))
            IDE_save(ed);
    }
    LOG_printf (LOG_DEBUG, "ide: IDE_exit -> exit(0)\n");
    exit(0);
}

static void show_help(IDE_editor ed)
{
    UI_EZRequest ("AQB Amiga QuickBasic Compiler IDE\n\nKeyboard shortcuts:\n\n"
                  "F1     - this help screen\n"
                  "S-UP   - page up\n"
                  "S-DOWN - page down\n"
                  "Ctrl-T - goto top of file\n"
                  "Ctrl-B - goto end of file\n"
                  "Ctrl-Y - delete line\n"
                  "F5     - compile & run\n"
                  "F7     - compile\n"
                  "Ctrl-S - save\n"
                  "Ctrl-C - quit", "Close");
    invalidateAll (ed);
}

static void compile(IDE_editor ed)
{
    // FIXME: save first

    LOG_printf (LOG_INFO, "\ncompilation starts...\n\n");

    CO_compile(ed->sourcefn,
               ed->module_name,
               /*symfn=*/ NULL,
               /*objfn=*/ NULL,
               ed->binfn,
               /*asm_gas_fn=*/ NULL,
               /*asm_asmpro_fn=*/ NULL,
               /*asm_vasm_fn=*/ NULL);

    LOG_printf (LOG_INFO, "\n*** press any key to continue ***\n\n");
    UI_waitkey ();

    if (EM_anyErrors)
    {
        gotoLine (ed, EM_firstErrorLine, EM_firstErrorCol);
        ed->il_show_error = TRUE;
    }

    UI_eraseDisplay ();
    invalidateAll (ed);
}

static void compileAndRun(IDE_editor ed)
{
    // FIXME: compile if not up to date

    LOG_printf (LOG_INFO, "\n");
    RUN_run (ed->binfn);

    LOG_printf (LOG_INFO, "\n*** press any key to continue ***\n\n");
    UI_waitkey ();

    UI_eraseDisplay ();
    invalidateAll (ed);
}

static void IDE_load_FileReq (IDE_editor ed)
{
    if (ed->editing)
        commitBuf (ed);
    if (ed->changed)
    {
        if (UI_EZRequest ("Save changes to disk?", "Yes|No"))
            IDE_save(ed);
    }

    UI_FileReq ("Load BASIC source code file");
}

static void size_cb (void *user_data)
{
	IDE_editor ed = (IDE_editor) user_data;
    invalidateAll (ed);
    initWindowSize (ed);
    invalidateAll (ed);
    scroll(ed);
    repaint(ed);
}

static void key_cb (uint16_t key, void *user_data)
{
	IDE_editor ed = (IDE_editor) user_data;

    switch (key)
    {
        case KEY_ESC:
        case KEY_CTRL_C:
        case KEY_CTRL_Q:
            IDE_exit(ed);

        case KEY_CTRL_Y:
            killLine(ed);
            break;

        case KEY_CURSOR_UP:
            cursorUp(ed);
            break;

        case KEY_CURSOR_DOWN:
            cursorDown(ed);
            break;

        case KEY_CURSOR_LEFT:
            cursorLeft(ed);
            break;

        case KEY_CURSOR_RIGHT:
            cursorRight(ed);
            break;

        case KEY_PAGE_UP:
            pageUp(ed);
            break;

        case KEY_PAGE_DOWN:
            pageDown(ed);
            break;

        case KEY_HOME:
            gotoHome(ed);
            break;

        case KEY_END:
            gotoEnd(ed);
            break;

        case KEY_GOTO_BOF:
            gotoBOF(ed);
            break;

        case KEY_GOTO_EOF:
            gotoEOF(ed);
            break;

        case KEY_ENTER:
            enterKey(ed);
            break;

        case KEY_BACKSPACE:
            backspaceKey(ed);
            break;

        case KEY_DEL:
            deleteKey(ed);
            break;

        case KEY_CTRL_O:
            IDE_load_FileReq(ed);
            break;

        case KEY_CTRL_S:
            IDE_save(ed);
            break;

        case KEY_HELP:
        case KEY_F1:
            show_help(ed);
            break;

        case KEY_F5:
        case KEY_CTRL_F:
            compileAndRun(ed);
            break;

        case KEY_F7:
        case KEY_CTRL_G:
            compile(ed);
            break;

		case KEY_COLORSCHEME_0:
		case KEY_COLORSCHEME_1:
		case KEY_COLORSCHEME_2:
		case KEY_COLORSCHEME_3:
		case KEY_COLORSCHEME_4:
		case KEY_COLORSCHEME_5:
			UI_setColorScheme (key - KEY_COLORSCHEME_0);
            invalidateAll (ed);
			break;

        case KEY_CUSTOMSCREEN:
            UI_setCustomScreen(!UI_isCustomScreen());
            invalidateAll (ed);
            break;

        case KEY_NONE:
            break;

        default:
            if (!insertChar(ed, (uint8_t) key))
                UI_bell();
            break;

    }

    scroll(ed);
    repaint(ed);
}

IDE_editor openEditor(void)
{
	IDE_editor ed = U_poolAlloc (UP_ide, sizeof (*ed));

    strncpy (ed->infoline, INFOLINE, sizeof(ed->infoline));
    ed->infoline_row     = 0;
    ed->sourcefn[0]      = 0;
    ed->module_name[0]   = 0;
    ed->line_first       = NULL;
    ed->line_last        = NULL;
    ed->num_lines	     = 0;
    ed->scrolloff_col    = 0;
    ed->scrolloff_row    = 0;
    ed->scrolloff_line   = NULL;
    ed->cursor_col		 = 0;
    ed->cursor_row		 = 0;
    ed->cursor_line      = NULL;
    ed->changed		     = FALSE;
    ed->editing          = FALSE;
    ed->il_show_error    = FALSE;

    invalidateAll (ed);

    initWindowSize(ed);

	return ed;
}

static void IDE_setSourceFn(IDE_editor ed, string sourcefn)
{
    if (sourcefn)
    {
        int l = strlen(sourcefn);
        if (l>PATH_MAX)
            l = PATH_MAX;

        strncpy (ed->sourcefn, sourcefn, PATH_MAX);

        if (l>4)
        {
            strcpy (ed->binfn, sourcefn);
            if (   (ed->binfn[l-4]=='.')
                && (ed->binfn[l-3]=='b')
                && (ed->binfn[l-2]=='a')
                && (ed->binfn[l-1]=='s'))
                ed->binfn[l-4]=0;
            else
                strcpy (ed->binfn, TMP_BINFN);
        }
        else
        {
            strcpy (ed->binfn, TMP_BINFN);
        }

        string module_name = basename(String(sourcefn));
        l = strlen(module_name);
        if (l>PATH_MAX)
            l = PATH_MAX;
        if (l>4)
            module_name[l-4] = 0;
        strncpy (ed->module_name, module_name, PATH_MAX);

        OPT_addModulePath(dirname(String(sourcefn)));
    }
    else
    {
        ed->sourcefn[0] = 0;
        ed->binfn[0]    = 0;
    }
}

static void loadSource (IDE_editor ed, string sourcefn)
{
    assert(!ed->num_lines); // FIXME: free old buffer

    IDE_setSourceFn(ed, sourcefn);

    if (sourcefn)
    {
        FILE *sourcef = fopen(sourcefn, "r");
        if (!sourcef)
        {
            fprintf(stderr, "failed to read %s: %s\n\n", sourcefn, strerror(errno));
            exit(2);
        }

        ed->buf_len = 0;
        bool eof = FALSE;
        bool eol = FALSE;
        IDE_line lastLine = NULL;
        while (!eof)
        {
            char ch;
            int n = fread (&ch, 1, 1, sourcef);
            if (n==1)
            {
                ed->buf[ed->buf_len++] = ch;
                if ( (ed->buf_len==(MAX_LINE_LEN-1)) || (ch==10)  )
                    eol = TRUE;
            }
            else
            {
                eof = TRUE;
                eol = TRUE;
            }

            if (eol)
            {
                ed->buf[ed->buf_len] = 0;
                IDE_line line = buf2line (ed);
                if (line->fold_start)
                    line->folded = TRUE;
                insertLineAfter (ed, lastLine, line);
                lastLine = line;
                eol=FALSE;
                ed->num_lines++;
                ed->buf_len = 0;
            }
        }

        fclose(sourcef);
    }
    else
    {
        ed->buf_len = 0;
        ed->buf[ed->buf_len] = 0;
        IDE_line line = buf2line (ed);
        insertLineAfter (ed, NULL, line);
        ed->num_lines++;
    }

    ed->cursor_col		 = 0;
    ed->cursor_row		 = 0;
    ed->cursor_line      = ed->line_first;

    indentSuccLines (ed, ed->line_first);
}

static void log_cb (uint8_t lvl, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    static char buf[1024];
    int l = vsnprintf (buf, 1024, fmt, args);
    va_end(args);
	if (lvl >= LOG_INFO)
    {
        //UI_scrollUp (/*fullscreen=*/TRUE);
        //UI_moveCursor (g_ed->window_height+1, 0);
        //UI_eraseToEOL ();
        //UI_moveCursor (g_ed->window_height, 0);

        uint16_t col = 0;
        for (int i =0; i<l; i++)
        {
            if (col >= g_ed->window_width)
            {
                UI_scrollUp (/*fullscreen=*/TRUE);
                UI_beginLine (g_ed->window_height+1);
                UI_endLine   ();
                col = 0;
            }
            char c = buf[i];
            if (c=='\n')
            {
                UI_scrollUp (/*fullscreen=*/TRUE);
                UI_beginLine (g_ed->window_height+1);
                UI_endLine   ();
            }
            else
            {
                UI_putc(c);
                col++;
            }
        }
    }
#if LOG_LEVEL == LOG_DEBUG
	fprintf (logf, "%s", buf);
	fflush (logf);
#endif
#ifdef LOG_SLOWDOWN
	fprintf (logf, "SLOWDOWN\n");
	fflush (logf);
    U_delay (50*50);
#endif
}

static void IDE_deinit(void)
{
    LOG_printf (LOG_DEBUG, "IDE_deinit.\n");
    U_deinit();
    UI_deinit();
#if LOG_LEVEL == LOG_DEBUG
	fclose (logf);
#endif
}

void IDE_open (string sourcefn)
{
    OPT_set (OPTION_VERBOSE, FALSE);
    UI_init();
    RUN_init();
#if LOG_LEVEL == LOG_DEBUG
    //printf ("opening %s ...\n", LOG_FILENAME);
    logf = fopen (LOG_FILENAME, "a");
    //printf ("opening %s ... done.\n", LOG_FILENAME);
#endif
	atexit (IDE_deinit);
    LOG_init (log_cb);

    // indentation support
    S_IF       = S_Symbol ("IF"      , FALSE);
    S_ELSEIF   = S_Symbol ("ELSEIF"  , FALSE);
    S_ELSE     = S_Symbol ("ELSE"    , FALSE);
    S_THEN     = S_Symbol ("THEN"    , FALSE);
    S_END      = S_Symbol ("END"     , FALSE);
    S_SUB      = S_Symbol ("SUB"     , FALSE);
    S_FUNCTION = S_Symbol ("FUNCTION", FALSE);
    S_FOR      = S_Symbol ("FOR"     , FALSE);
    S_NEXT     = S_Symbol ("NEXT"    , FALSE);
    S_DO       = S_Symbol ("DO"      , FALSE);
    S_LOOP     = S_Symbol ("LOOP"    , FALSE);
    S_WHILE    = S_Symbol ("WHILE"   , FALSE);
    S_WEND     = S_Symbol ("WEND"    , FALSE);
    S_SELECT   = S_Symbol ("SELECT"  , FALSE);
    S_CASE     = S_Symbol ("CASE"    , FALSE);

    g_keywords = TAB_empty (UP_ide);
    for (int i =0; i<FE_num_keywords; i++)
        TAB_enter (g_keywords, FE_keywords[i], (void *) TRUE);

    g_ed = openEditor();

    loadSource (g_ed, sourcefn);

    UI_setCursorVisible (FALSE);
    UI_moveCursor (0, 0);
    UI_eraseDisplay();
    repaint(g_ed);

	UI_onKeyCall(key_cb, g_ed);
    UI_onSizeChangeCall (size_cb, g_ed);

	UI_run();
}

