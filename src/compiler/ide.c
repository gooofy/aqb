#include "ide.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "util.h"
#include "terminal.h"
#include "scanner.h"
#include "frontend.h"

//#define ENABLE_DEBUG

#define STYLE_NORMAL 0
#define STYLE_KW     1
#define STYLE_STRING 2
#define STYLE_NUMBER 3

#define INFOLINE            "X=   1 Y=   1 #=   0  new file"
#define INFOLINE_CURSOR_X      2
#define INFOLINE_CURSOR_Y      9
#define INFOLINE_NUM_LINES    16
#define INFOLINE_CHANGED      21
#define INFOLINE_FILENAME     22

#define CR  13
#define LF  10
#define TAB  9

#define SCROLL_MARGIN 5

typedef struct IDE_line_     *IDE_line;
typedef struct IDE_editor_   *IDE_editor;

struct IDE_line_
{
	IDE_line  next, prev;
	uint16_t  len;
    char     *buf;
    char     *style;
};

struct IDE_editor_
{
    IDE_line           line_first, line_last;
	uint16_t           num_lines;
	uint16_t           window_width, window_height;
	uint16_t           scrolloff_col, scrolloff_row;
	bool               changed;
	char               infoline[36+PATH_MAX];
    uint16_t           infoline_row;
	char 	           filename[PATH_MAX];

	uint16_t           cursor_col, cursor_row;
    IDE_line           cursor_line;

    // cursor_line currently being edited (i.e. represented in buffer):
    bool               editing;
    char               buf[MAX_LINE_LEN];
    char               style[MAX_LINE_LEN];
    uint16_t           buf_len;
    uint16_t           buf_pos;
};

#define LOG_FILENAME "ide.log"

#ifdef ENABLE_DEBUG
static FILE *logf=NULL;

static void lprintf (const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vfprintf(logf, format, argptr);
    va_end(argptr);
	fflush(logf);
}
#endif

IDE_line newLine(IDE_editor ed, char *buf, char *style)
{
    int len = strlen(buf);
    IDE_line l = U_poolAlloc (UP_ide, sizeof(*l)+2*(len+1));

	l->next  = NULL;
	l->prev  = NULL;
	l->len   = len;
    l->buf   = (char *) (&l[1]);
    l->style = l->buf + len + 1;

    memcpy (l->buf, buf, len+1);
    memcpy (l->style, style, len+1);

	return l;
}

static void freeLine (IDE_editor ed, IDE_line l)
{
    // FIXME: implement
}

static void IDE_lineListInsertAfter (IDE_editor ed, IDE_line lBefore, IDE_line l)
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
            ed->line_first = l;
        }
        else
        {
            ed->line_first = ed->line_last = l;
        }
    }
}

void initWindowSize (IDE_editor ed)
{
    TE_setAlternateScreen(TRUE);

    int rows, cols;
    if (!TE_getsize (&rows, &cols))
        exit(1);

    ed->window_width  = cols;
    ed->window_height = rows-1;
    ed->infoline_row  = rows-2;
}

static void outputInfoLine(IDE_editor ed)
{
#ifdef ENABLE_DEBUG
	lprintf ("outputInfoLine: row=%d, txt=%s\n", ed->infoline_row, ed->infoline);
#endif
    TE_setTextStyle (TE_STYLE_NORMAL);
    TE_setTextStyle (TE_STYLE_INVERSE);
    TE_moveCursor   (ed->infoline_row+1, 1);
    char *c = ed->infoline;
    int col = 1;
    while (*c && col < ed->window_width)
    {
        TE_putc (*c++);
        col++;
    }
    while (col < ed->window_width)
    {
        TE_putc (' ');
        col++;
    }
    TE_setTextStyle (TE_STYLE_NORMAL);
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

static void showXPos(IDE_editor ed)
{
    _itoa (ed->cursor_col, ed->infoline + INFOLINE_CURSOR_X, 4);
    outputInfoLine (ed);
}
static void showYPos(IDE_editor ed)
{
    _itoa (ed->cursor_row, ed->infoline + INFOLINE_CURSOR_Y, 4);
    outputInfoLine (ed);
}
static void showNumLines(IDE_editor ed)
{
    _itoa (ed->num_lines, ed->infoline + INFOLINE_NUM_LINES, 4);
    outputInfoLine (ed);
}
static void showFlags(IDE_editor ed)
{
	char *fs = ed->infoline + INFOLINE_CHANGED;

	if (ed->changed)
		*fs++ = '*';
	else
		*fs++ = ' ';
    outputInfoLine (ed);
}
static void showFilename(IDE_editor ed)
{
	char *fs = ed->infoline + INFOLINE_FILENAME;

    for (char *c = ed->filename; *c; c++)
    {
        *fs++ = *c;
    }

    *fs = 0;
    outputInfoLine (ed);
}

static void showInfoLine(IDE_editor ed)
{
	showXPos(ed);
	showYPos(ed);
	showNumLines(ed);
	showFlags(ed);
	showFilename(ed);
}

static void showCursor (IDE_editor ed)
{
    TE_moveCursor (ed->cursor_row-ed->scrolloff_row+1, ed->cursor_col-ed->scrolloff_col+1);
    TE_setCursorVisible (TRUE);
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

static void showLine (IDE_editor ed, char *buf, char *style, int len, int row)
{
    TE_moveCursor (row, 1);

    char s=STYLE_NORMAL;
    TE_setTextStyle (TE_STYLE_NORMAL);
    for (int i=0; i<len; i++)
    {
        char s2 = style[i];
        if (s2 != s)
        {
            switch (s2)
            {
                case STYLE_NORMAL:
                    TE_setTextStyle (TE_STYLE_NORMAL);
                    break;
                case STYLE_KW:
                    TE_setTextStyle (TE_STYLE_BOLD);
                    TE_setTextStyle (TE_STYLE_YELLOW);
                    break;
                case STYLE_STRING:
                    TE_setTextStyle (TE_STYLE_BOLD);
                    TE_setTextStyle (TE_STYLE_MAGENTA);
                    break;
                case STYLE_NUMBER:
                    TE_setTextStyle (TE_STYLE_BOLD);
                    TE_setTextStyle (TE_STYLE_MAGENTA);
                    break;
                default:
                    assert(FALSE);
            }
            s = s2;
        }
        TE_putc (buf[i]);
    }
    TE_eraseToEOL();
}

static void scrollX(IDE_editor ed)
{
    // FIXME: implement
}

static void scrollY(IDE_editor ed)
{
    // scroll up ?

    uint16_t cy = ed->cursor_row-ed->scrolloff_row;
    if ( (cy > ed->window_height - SCROLL_MARGIN) && (ed->scrolloff_row < ed->num_lines - ed->window_height) )
    {
        ed->scrolloff_row++;
        TE_scrollUp();

        int linenum = ed->scrolloff_row + ed->window_height - 2;
        IDE_line l = getLine (ed, linenum);
        showLine (ed, l->buf, l->style, l->len, ed->window_height-1);
        showInfoLine(ed);
    }
}

static bool nextch_cb(char *ch, void *user_data)
{
    IDE_editor ed = user_data;
    *ch = ed->buf[ed->buf_pos++];
    return (*ch) != 0;
}

static IDE_line buf2line (IDE_editor ed)
{
    static char buf[MAX_LINE_LEN];
    static char style[MAX_LINE_LEN];

    ed->buf_pos = 0;
    ed->buf[ed->buf_len]=0;
    S_init (nextch_cb, ed);

    S_tkn tkn = S_nextline();
    int pos = 0;
    bool first = TRUE;
    S_token lastKind = S_ERRTKN;
    while (tkn && (pos <MAX_LINE_LEN-1))
    {
        switch (tkn->kind)
        {
            case S_ERRTKN:
            case S_EOL:
                break;
            case S_IDENT:
            {
                if (!first)
                    buf[pos++] = ' ';
                bool is_kw = FALSE;
                for (int i =0; i<FE_num_keywords; i++)
                {
                    if (FE_keywords[i]==tkn->u.sym)
                    {
                        is_kw = TRUE;
                        break;
                    }
                }
                char *s = S_name(tkn->u.sym);
                int l = strlen(s);
                if (pos+l >= MAX_LINE_LEN-1)
                    l = MAX_LINE_LEN-1-pos;
                for (int i =0; i<l; i++)
                {
                    if (is_kw)
                    {
                        buf[pos] = toupper(s[i]);
                        style[pos++] = STYLE_KW;
                    }
                    else
                    {
                        buf[pos] = s[i];
                        style[pos++] = STYLE_NORMAL;
                    }
                }
                break;
            }
            case S_STRING:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '"';
                style[pos++] = STYLE_STRING;
                for (char *c=tkn->u.str; *c; c++)
                {
                    buf[pos] = *c;
                    style[pos++] = STYLE_STRING;
                }
                buf[pos] = '"';
                style[pos++] = STYLE_STRING;
                break;
            case S_COLON:
                buf[pos] = ':';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_SEMICOLON:
                buf[pos] = ';';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_COMMA:
                buf[pos] = ',';
                style[pos++] = STYLE_NORMAL;
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
                    style[pos++] = STYLE_NUMBER;
                }
                break;
            }
            case S_FNUM:
            {
                if (!first && (lastKind != S_MINUS))
                    buf[pos++] = ' ';
                static char nbuf[64];
                snprintf (nbuf, 64, "%g", tkn->u.literal.fnum);
                for (char *c=nbuf; *c; c++)
                {
                    buf[pos] = *c;
                    style[pos++] = STYLE_NUMBER;
                }
                break;
            }
            case S_MINUS:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '-';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_LPAREN:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '(';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_RPAREN:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = ')';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_EQUALS:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '=';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_EXP:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '^';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_ASTERISK:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '*';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_SLASH:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '/';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_BACKSLASH:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '\\';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_PLUS:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '+';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_GREATER:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '>';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_LESS:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '<';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_NOTEQ:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '<';
                style[pos++] = STYLE_NORMAL;
                buf[pos] = '>';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_LESSEQ:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '<';
                style[pos++] = STYLE_NORMAL;
                buf[pos] = '=';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_GREATEREQ:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '>';
                style[pos++] = STYLE_NORMAL;
                buf[pos] = '=';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_POINTER:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '-';
                style[pos++] = STYLE_NORMAL;
                buf[pos] = '>';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_PERIOD:
                buf[pos] = '.';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_AT:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '@';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_LBRACKET:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '[';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_RBRACKET:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = ']';
                style[pos++] = STYLE_NORMAL;
                break;
            case S_TRIPLEDOTS:
                if (!first)
                    buf[pos++] = ' ';
                buf[pos] = '.';
                style[pos++] = STYLE_NORMAL;
                buf[pos] = '.';
                style[pos++] = STYLE_NORMAL;
                buf[pos] = '.';
                style[pos++] = STYLE_NORMAL;
                break;
        }

        lastKind = tkn->kind;
        tkn = tkn->next;
        first = FALSE;
    }
    buf[pos] = 0;

    return newLine (ed, buf, style);
}

static bool cursorDown(IDE_editor ed)
{
    IDE_line nl = ed->cursor_line->next;
    if (!nl)
        return FALSE;

    IDE_line cl = ed->cursor_line;
    if (ed->editing)
    {
        IDE_line l = buf2line (ed);
        if (cl->prev)
            cl->prev->next = l;
        else
            ed->line_first = l;
        nl->prev = cl->prev;
        if (cl->next)
            cl->next->prev = l;
        else
            ed->line_last = l;
        nl = l->next = cl->next;
        ed->editing = FALSE;
        freeLine (ed, cl);
        showLine (ed, l->buf, l->style, l->len, ed->cursor_row - ed->scrolloff_row + 1);
    }

    ed->cursor_line = nl;
    ed->cursor_row++;
    showYPos(ed);
    scrollY(ed);
    if (ed->cursor_col > nl->len)
    {
        ed->cursor_col = nl->len;
        showXPos(ed);
        scrollX(ed);
    }
    showCursor(ed);

    return TRUE;
}

static bool cursorRight(IDE_editor ed)
{
    int len = ed->editing ? ed->buf_len : ed->cursor_line->len;
    if (ed->cursor_col >= len)
        return FALSE;

    ed->cursor_col++;
    showXPos(ed);
    scrollX(ed);
    showCursor(ed);

    return TRUE;
}

static void line2buf (IDE_editor ed, IDE_line l)
{
    memcpy (ed->buf,   l->buf  , l->len+1);
    memcpy (ed->style, l->style, l->len+1);
    ed->editing  = TRUE;
    if (!ed->changed)
    {
        ed->changed  = TRUE;
        showFlags(ed);
    }
    ed->buf_len  = l->len;
}

static bool printableAsciiChar (uint8_t c)
{
    return (c >= 32) && (c <= 126);
}

static bool insertChar (IDE_editor ed, uint8_t c)
{
    if (!printableAsciiChar(c))
        return FALSE;

    if (!ed->editing)
    {
        line2buf (ed, ed->cursor_line);
    }

    uint16_t cp = ed->scrolloff_col + ed->cursor_col;
    for (uint16_t i=ed->buf_len; i>cp; i--)
    {
        ed->buf[i]   = ed->buf[i-1];
        ed->style[i] = ed->style[i-1];
    }
    ed->buf[cp]   = c;
    ed->style[cp] = STYLE_NORMAL;
    ed->buf_len++;

    showLine (ed, ed->buf, ed->style, ed->buf_len, ed->cursor_row - ed->scrolloff_row + 1);

    cursorRight(ed);

    return TRUE;
}

// handle keypress, return value TRUE to keep editor running, FALSE to quit
static bool handleKey (IDE_editor ed, int key)
{

    switch (key)
    {
        case KEY_ESC:
            return FALSE;

        case KEY_CURSOR_DOWN:
            cursorDown(ed);
            break;

        case KEY_CURSOR_RIGHT:
            cursorRight(ed);
            break;

        default:
            if (!insertChar(ed, (uint8_t) key))
                TE_bell();
            break;

    }

    TE_flush();
    return TRUE;
}

IDE_editor OpenEditor(void)
{
	IDE_editor ed = U_poolAlloc (UP_ide, sizeof (*ed));

    strncpy (ed->infoline, INFOLINE, sizeof(ed->infoline));
    ed->infoline_row     = 0;
    ed->filename[0]      = 0;
    ed->line_first       = NULL;
    ed->line_last        = NULL;
    ed->num_lines	     = 0;
    ed->scrolloff_col    = 0;
    ed->scrolloff_row    = 0;
    ed->cursor_col		 = 0;
    ed->cursor_row		 = 0;
    ed->cursor_line      = NULL;
    ed->changed		     = FALSE;
    ed->editing          = FALSE;

    initWindowSize(ed);

	return ed;
}

static void IDE_exit(void)
{
    TE_moveCursor(0, 0);
    TE_eraseDisplay();
    TE_setAlternateScreen(FALSE);
    TE_flush();
}

static void showAll (IDE_editor ed)
{
    IDE_line l = getLine (ed, ed->scrolloff_row);

    int linenum_end = ed->scrolloff_row + ed->window_height - 2;
    int linenum = ed->scrolloff_row;
    while (l && (linenum <= linenum_end))
    {
        showLine (ed, l->buf, l->style, l->len, linenum - ed->scrolloff_row + 1);
        linenum++;
        l = l->next;
    }
}

static void IDE_load (IDE_editor ed, char *sourcefn)
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
    int linenum = 0;
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
            IDE_lineListInsertAfter (ed, lastLine, line);
            lastLine = line;
            linenum = (linenum+1) % ed->window_height;
            eol=FALSE;
            ed->num_lines++;
            ed->buf_len = 0;
        }
    }

    fclose(sourcef);

    strcpy (ed->filename, sourcefn);
    ed->cursor_col		 = 0;
    ed->cursor_row		 = 0;
    ed->cursor_line      = ed->line_first;
}

void IDE_open(char *fn)
{
    TE_init();
#ifdef ENABLE_DEBUG
    logf = fopen (LOG_FILENAME, "w");
#endif

    atexit (IDE_exit);

	IDE_editor ed = OpenEditor();

    if (fn)
        IDE_load (ed, fn);

    TE_setCursorVisible (FALSE);
    TE_moveCursor (0, 0);
    TE_eraseDisplay();
    showAll(ed);
    showInfoLine(ed);
    showCursor(ed);
    TE_flush();

    bool running = TRUE;
    while (running)
    {
        int ch = TE_getch();
        running = handleKey(ed, ch);
    }
}

