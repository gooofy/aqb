#include "ide.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "util.h"
#include "terminal.h"
#include "scanner.h"
#include "frontend.h"

#define STYLE_NORMAL 0
#define STYLE_KW     1
#define STYLE_STRING 2
#define STYLE_NUMBER 3

#define INFOLINE            "X=   1 Y=   1 #=   0 IcATB"
#define INFOLINE_CURSOR_X      2
#define INFOLINE_CURSOR_Y      9
#define INFOLINE_NUM_LINES    16
#define INFOLINE_FLAGS        21

#define CR  13
#define LF  10
#define TAB  9

typedef struct IDE_line_     *IDE_line;
typedef struct IDE_lineList_  IDE_lineList;
typedef struct IDE_editor_   *IDE_editor;

struct IDE_line_
{
	IDE_line  next, prev;
	uint16_t  len;
    char     *buf;
    char     *style;
};

struct IDE_lineList_
{
	IDE_line first;
	IDE_line last;
};

struct IDE_editor_
{
	IDE_lineList       lines;
	uint16_t           num_lines;
	uint16_t           window_width, window_height;
	uint16_t           scrolloff_col, scrolloff_row;
	uint16_t           cursor_col, cursor_row;
	bool               changed;
	bool               insert;
	bool               tabs;
	bool               skipblanks;
	bool               autoindent;
	char               infoline[36];
    uint16_t           infoline_row;
	uint8_t	           filename[PATH_MAX];
    char               buf[MAX_LINE_LEN];
    uint16_t           buf_pos;
};

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

static void IDE_lineListInsertAfter (IDE_lineList *ll, IDE_line lBefore, IDE_line l)
{
    if (lBefore)
    {
        if (lBefore == ll->last)
        {
            ll->last = l;
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
        l->next = ll->first;
        if (ll->first)
        {
            ll->first = l;
        }
        else
        {
            ll->first = ll->last = l;
        }
    }
}

void initWindowSize (IDE_editor ed)
{
    int rows, cols;
    if (!TE_getsize (&rows, &cols))
        exit(1);

    ed->window_width          = cols;
    ed->window_height	         = rows-1;
    ed->infoline_row = rows-1;
}

static void updateInfoLine(IDE_editor ed)
{
    TE_setTextStyle (TE_STYLE_INVERSE);
    TE_moveCursor   (ed->infoline_row, 0);
    TE_eraseToEOL   ();
    TE_putstr       (ed->infoline);
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
    updateInfoLine (ed);
}
static void showYpos(IDE_editor ed)
{
    _itoa (ed->cursor_row, ed->infoline + INFOLINE_CURSOR_Y, 4);
    updateInfoLine (ed);
}
static void showNumLines(IDE_editor ed)
{
    _itoa (ed->num_lines, ed->infoline + INFOLINE_NUM_LINES, 4);
    updateInfoLine (ed);
}
static void showFlags(IDE_editor ed)
{
	char *fs = ed->infoline + INFOLINE_FLAGS;

	if (ed->insert)
		*fs++ = 'I';
	else
		*fs++ = 'O';
	if (ed->changed)
		*fs++ = 'C';
	else
		*fs++ = 'c';
	if (ed->autoindent)
		*fs++ = 'A';
	else
		*fs++ = 'a';
	if (ed->tabs)
		*fs++ = 'T';
	else
		*fs++ = 't';
	if (ed->skipblanks)
		*fs = 'B';
	else
		*fs = 'b';
    updateInfoLine (ed);
}

static void showInfoLine(IDE_editor ed)
{
	showXPos(ed);
	showYpos(ed);
	showNumLines(ed);
	showFlags(ed);
}

#if 0
static bool cursorRight(IDE_editor ed)
{
	if (ed->cursor_col < TE_MAX_COLUMNS)
	{
		ed->cursor_col++;
		if (ed->cursor_col >= (ed->scrolloff_col + ed->window_width))
			scrollLeft (ed, 1);

		showXPos(ed);
		return TRUE;
	}
	else
		return FALSE;
}
#endif

static bool insertChar (IDE_editor ed, uint8_t c)
{
    assert(FALSE);
#if 0

	// move cursor
	while (xadd)
	{
		if (!cursorRight(ed))
            break;
		xadd--;
	}

	outputLine (ed, ed->cur_line, ed->yoff + ed->ch*ed->wdy);
	return TRUE;
#endif
    return FALSE;
}

// handle keypress, return value TRUE to keep editor running, FALSE to quit
static bool handleKey (IDE_editor ed, int key)
{

    switch (key)
    {
        case KEY_ESC:
            return FALSE;

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
    ed->infoline_row     = 1;
    ed->filename[0]      = 0;
    ed->lines.first      = NULL;
    ed->lines.last       = NULL;
    ed->num_lines	     = 0;
    ed->scrolloff_col    = 0;
    ed->scrolloff_row    = 0;
    ed->cursor_col		 = 1;
    ed->cursor_row		 = 1;
    ed->changed		     = 0;
    ed->insert		     = 1;
    ed->tabs			 = 1;
    ed->skipblanks	     = 1;
    ed->autoindent	     = 1;

    initWindowSize(ed);

	return ed;
}

static void IDE_exit(void)
{
    TE_moveCursor (0, 0);
    TE_eraseDisplay();
    TE_flush();
}

static bool nextch_cb(char *ch, void *user_data)
{
    IDE_editor ed = user_data;
    *ch = ed->buf[ed->buf_pos++];
    return (*ch) != 0;
}

static IDE_line buf2line (IDE_editor ed, int linenum)
{
    static char buf[MAX_LINE_LEN];
    static char style[MAX_LINE_LEN];

    ed->buf_pos = 0;
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

static void showLine (IDE_editor ed, IDE_line l, int row)
{
    TE_moveCursor (row, 0);

    char s=STYLE_NORMAL;
    TE_setTextStyle (TE_STYLE_NORMAL);
    for (int i=0; i<l->len; i++)
    {
        char s2 = l->style[i];
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
        TE_putc (l->buf[i]);
    }
    TE_eraseToEOL();
}

static void showAll (IDE_editor ed)
{
    IDE_line l = ed->lines.first;
    int linenum = 0;
    while ( l && (linenum < ed->scrolloff_row) )
    {
        linenum++;
        l = l->next;
    }

    int linenum_end = ed->scrolloff_row + ed->window_height;
    while (l && (linenum < linenum_end))
    {
        showLine (ed, l, linenum - ed->scrolloff_row);
        linenum++;
        l = l->next;
    }
}

static void showCursor (IDE_editor ed)
{
    TE_moveCursor (ed->cursor_col-ed->scrolloff_col-1, ed->cursor_row-ed->scrolloff_row-1);
    TE_setCursorVisible (TRUE);
}

static void IDE_load (IDE_editor ed, char *sourcefn)
{
	FILE *sourcef = fopen(sourcefn, "r");
	if (!sourcef)
	{
		fprintf(stderr, "failed to read %s: %s\n\n", sourcefn, strerror(errno));
		exit(2);
	}

    int l = 0;
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
            ed->buf[l++] = ch;
            if ( (l==(MAX_LINE_LEN-1)) || (ch==10)  )
                eol = TRUE;
        }
        else
        {
            eof = TRUE;
            eol = TRUE;
        }

        if (eol)
        {
            ed->buf[l] = 0;
            l = 0;
            IDE_line line = buf2line (ed, linenum);
            IDE_lineListInsertAfter (&ed->lines, lastLine, line);
            lastLine = line;
            linenum = (linenum+1) % ed->window_height;
            eol=FALSE;
        }
    }

    fclose(sourcef);
}

void IDE_open(char *fn)
{
    TE_init();

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

