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
#include <sys/stat.h>

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
#define INFOLINE_CHANGED      38
#define INFOLINE_FILENAME     30

#define CR  13
#define LF  10
#define TAB  9

#define SCROLL_MARGIN 5

#define INDENT_SPACES 4

#ifdef __amigaos__
#define TMP_BINFN "T:aqb_out.exe"
#include "icons.h"
#include <clib/icon_protos.h>
#include <inline/icon.h>
extern struct IconBase      *IconBase;
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
static S_symbol S_REM;

#if LOG_LEVEL == LOG_DEBUG
static FILE *logf=NULL;
#endif
static IDE_instance g_ed;
static TAB_table  g_keywords;

IDE_line newLine(IDE_instance ed, char *buf, char *style, int8_t pre_indent, int8_t post_indent, bool fold_start, bool fold_end)
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
    l->a_line      = 0;
    l->v_line      = 0;

    memcpy (l->buf, buf, len+1);
    memcpy (l->style, style, len+1);

	return l;
}

static void freeLine (IDE_instance ed, IDE_line l)
{
    // FIXME: implement
    ed->scrolloff_line = NULL;
}

static void insertLineAfter (IDE_instance ed, IDE_line lBefore, IDE_line l)
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

static void deleteLine (IDE_instance ed, IDE_line l)
{
    if (l->prev)
    {
        l->prev->next = l->next;
    }
    else
    {
        ed->line_first = l->next;
        ed->line_first->a_line=0;
        ed->line_first->v_line=0;
    }
    if (l->next)
        l->next->prev = l->prev;
    else
        ed->line_last = l->prev;
    freeLine (ed, l);
    ed->changed = TRUE;
}

void initWindowSize (IDE_instance ed)
{
    ed->window_width  = UI_size_cols;
    ed->window_height = UI_size_rows;
    ed->infoline_row  = UI_size_rows-1;
    UI_setScrollArea (1, UI_size_rows-1);
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

static IDE_line getLine (IDE_instance ed, int linenum)
{
    IDE_line l = ed->line_first;
    while ( l && (l->v_line < linenum) )
        l = l->next;
    return l;
}

static void invalidateAll (IDE_instance ed)
{
    for (uint16_t i=0; i<UI_MAX_ROWS; i++)
        ed->up2date_row[i] = FALSE;
    ed->up2date_il_pos       = FALSE;
    ed->up2date_il_num_lines = FALSE;
    ed->up2date_il_flags     = FALSE;
    ed->up2date_il_sourcefn  = FALSE;
    ed->repaint_all          = TRUE;
}

static void scroll(IDE_instance ed)
{
    int16_t cursor_row = ed->cursor_v_line - ed->scrolloff_row;

    // scroll up ?

    if (cursor_row > ed->window_height - SCROLL_MARGIN)
    {
        int16_t scrolloff_row_new = ed->cursor_v_line - ed->window_height + SCROLL_MARGIN;
        int16_t scrolloff_row_max = ed->num_vlines - ed->window_height + 1;

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
        int16_t scrolloff_row_new = ed->cursor_v_line > SCROLL_MARGIN ? ed->cursor_v_line - SCROLL_MARGIN : 0;

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

typedef enum { STAUI_START, STAUI_IF, STAUI_ELSEIF, STAUI_ELSE, STAUI_THEN,
               STAUI_ELSEIFTHEN, STAUI_LOOP, STAUI_LOOPEND,
               STAUI_END, STAUI_SUB, STAUI_SELECT, STAUI_CASE, STAUI_OTHER } state_enum;

static void _getch(IDE_instance ed)
{
    assert (!ed->eol);
    ed->buf_ch = ed->buf[ed->buf_pos++];
    ed->eol = ed->buf_ch == 0;
}

static IDE_line buf2line (IDE_instance ed)
{
    static char buf[MAX_LINE_LEN];
    static char style[MAX_LINE_LEN];
    uint16_t    pos         = 0;
    int8_t      pre_indent  = 0;
    int8_t      post_indent = 0;
    bool        fold_start  = FALSE;
    bool        fold_end    = FALSE;
    state_enum  state       = STAUI_START;
    bool        first       = TRUE;
    S_symbol    last_sym    = NULL;

    ed->buf_pos             = 0;
    ed->buf[ed->buf_len]    = 0;
    ed->eol                 = FALSE;

    _getch(ed);

    while (TRUE)
    {
        while (!ed->eol && (S_isWhitespace(ed->buf_ch) || (ed->buf_ch=='\n') || (ed->buf_ch=='\r')) )
        {
            if (!first && (ed->buf_ch!='\n') && (ed->buf_ch!='\r') )
            {
                buf[pos] = ed->buf_ch;
                style[pos++] = UI_TEXT_STYLE_TEXT;
            }
            _getch(ed);
        }

        //printf ("last_sym=%s ch=%d\n", last_sym ? S_name(last_sym) : "NULL", ed->buf_ch);
        if (ed->eol || ed->buf_ch == ':')
        {
            //printf ("eol/: state=%d, last_sym=%s\n", state, last_sym ? S_name(last_sym) : "NULL");
            // used to have a switch statement here, but m68k gcc became horribly slow
            if (state == STAUI_THEN)
            {
                if (last_sym == S_THEN)
                {
                    post_indent++;
                }
            }
            else if (state == STAUI_ELSEIFTHEN)
            {
                if (last_sym == S_THEN)
                {
                    pre_indent--;
                    post_indent++;
                }
            }
            else if (state == STAUI_ELSE)
            {
                if (last_sym == S_ELSE)
                {
                    pre_indent--;
                    post_indent++;
                }
            }
            else if ((state == STAUI_SUB) || (state == STAUI_SELECT))
            {
                post_indent++;
            }
            else if (state == STAUI_END)
            {
                pre_indent--;
            }
            else if (state == STAUI_CASE)
            {
                pre_indent--;
                post_indent++;
            }
            state = STAUI_START;
        }

        if (ed->eol)
            break;

        first = FALSE;

        // handle line comments
        if (ed->buf_ch == '\'')
        {
            while (!ed->eol)
            {
                if ((ed->buf_ch!='\n') && (ed->buf_ch!='\r') )
                {
                    buf[pos] = ed->buf_ch;
                    style[pos++] = UI_TEXT_STYLE_COMMENT;
                }
                _getch(ed);
            }
            break;
        }

        // handle strings
        if (ed->buf_ch == '"')
        {
            do
            {
                buf[pos] = ed->buf_ch;
                style[pos++] = UI_TEXT_STYLE_TEXT;
                _getch(ed);
            } while (!ed->eol && ed->buf_ch != '"');
            if (!ed->eol)
            {
                buf[pos] = ed->buf_ch;
                style[pos++] = UI_TEXT_STYLE_TEXT;
                _getch(ed);
            }
            continue;
        }

        // identifiers is what we're really interested in
        // (for highlighting + identing + folding)
        if (S_isIDStart(ed->buf_ch))
        {

            static char idbuf[MAX_LINE_LEN];

            int l = 0;

            idbuf[l] = ed->buf_ch;
            l++;
            _getch(ed);
            while (S_isIDCont(ed->buf_ch) && !ed->eol)
            {
                idbuf[l] = ed->buf_ch;
                l++;
                _getch(ed);
            }

            // type marker?
            switch (ed->buf_ch)
            {
                case '%':
                case '&':
                case '!':
                case '#':
                case '$':
                    idbuf[l] = ed->buf_ch;
                    l++;
                    _getch(ed);
                    break;
            }
            idbuf[l] = '\0';

            S_symbol sym = S_Symbol (idbuf, /*case_sensitive=*/FALSE);
            if (sym == S_REM)
            {
                for (int i =0; i<l; i++)
                {
                    buf[pos] = toupper(idbuf[i]);
                    style[pos++] = UI_TEXT_STYLE_COMMENT;
                }
                while (!ed->eol)
                {
                    if ((ed->buf_ch!='\n') && (ed->buf_ch!='\r') )
                    {
                        buf[pos] = ed->buf_ch;
                        style[pos++] = UI_TEXT_STYLE_COMMENT;
                    }
                    _getch(ed);
                }
            }
            else
            {
                bool is_kw = TAB_look(g_keywords, sym) != NULL;
                for (int i =0; i<l; i++)
                {
                    if (is_kw)
                    {
                        buf[pos] = toupper(idbuf[i]);
                        style[pos++] = UI_TEXT_STYLE_KEYWORD;
                    }
                    else
                    {
                        buf[pos] = idbuf[i];
                        style[pos++] = UI_TEXT_STYLE_TEXT;
                    }
                }

                // auto-indentation is based on very crude BASIC parsing

                switch (state)
                {
                    case STAUI_START:
                        if (sym == S_IF)
                        {
                            state = STAUI_IF;
                        }
                        else if (sym == S_ELSEIF)
                        {
                            state = STAUI_ELSEIF;
                        }
                        else if (sym == S_ELSE)
                        {
                            state = STAUI_ELSE;
                        }
                        else if (sym == S_END)
                        {
                            state = STAUI_END;
                        }
                        else if (sym == S_SUB)
                        {
                            state = STAUI_SUB;
                            fold_start = TRUE;
                        }
                        else if (sym == S_FUNCTION)
                        {
                            state = STAUI_SUB;
                            fold_start = TRUE;
                        }
                        else if (sym == S_FOR)
                        {
                            state = STAUI_LOOP;
                            post_indent++;
                        }
                        else if (sym == S_DO)
                        {
                            state = STAUI_LOOP;
                            post_indent++;
                        }
                        else if (sym == S_WHILE)
                        {
                            state = STAUI_LOOP;
                            post_indent++;
                        }
                        else if ((sym == S_NEXT) || (sym == S_LOOP) || (sym == S_WEND))
                        {
                            state = STAUI_LOOPEND;
                            if (post_indent>0)
                                post_indent--;
                            else
                                pre_indent--;
                        }
                        else if (sym == S_SELECT)
                        {
                            state = STAUI_SELECT;
                        }
                        else if (sym == S_CASE)
                        {
                            state = STAUI_CASE;
                        }
                        else
                        {
                            state = STAUI_OTHER;
                        }
                        break;
                    case STAUI_IF:
                        if (sym == S_THEN)
                        {
                            state = STAUI_THEN;
                        }
                        break;
                    case STAUI_ELSEIF:
                        if (sym == S_THEN)
                        {
                            state = STAUI_ELSEIFTHEN;
                        }
                        break;
                    case STAUI_END:
                        if ((sym == S_SUB) || (sym == S_FUNCTION))
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
                    case STAUI_OTHER:
                        break;
                    default:
                        assert(FALSE);
                }
            }
            last_sym = sym;
        }
        else
        {
            buf[pos] = ed->buf_ch;
            style[pos++] = UI_TEXT_STYLE_TEXT;
            _getch(ed);
            last_sym = NULL;
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

static void indentSuccLines (IDE_instance ed, IDE_line lp)
{
    uint16_t al = lp->a_line;
    uint16_t vl = lp->v_line;
    IDE_line l = lp->next;
    while (l)
    {
        indentLine (l);
        al++;
        if (l->fold_start || !l->folded)
            vl++;
        l->a_line = al;
        l->v_line = vl;
        l = l->next;
    }
    ed->num_lines = al+1;
    ed->num_vlines = vl+1;
    invalidateAll (ed);
}

static IDE_line commitBuf(IDE_instance ed)
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
    l->a_line = cl->a_line;
    l->v_line = cl->v_line;
    ed->editing = FALSE;
    int8_t old_indent = cl->indent;
    int8_t old_post_indent = cl->post_indent;
    freeLine (ed, cl);
    ed->cursor_line = l;
    indentLine (l);
    if ( (l->indent == old_indent) && (l->post_indent == old_post_indent) )
        ed->up2date_row[ed->cursor_v_line - ed->scrolloff_row] = FALSE;
    else
        indentSuccLines (ed, l);
    return l;
}

static bool cursorUp(IDE_instance ed)
{
    IDE_line pl = ed->cursor_line->prev;
    if (!pl)
        return FALSE;

    if (ed->editing)
        commitBuf (ed);

    if (pl->folded)
    {
        while (pl->prev && !pl->fold_start)
        {
            pl = pl->prev;
        };
    }
    ed->cursor_line = pl;
    ed->cursor_a_line = pl->a_line;
    ed->cursor_v_line = pl->v_line;
    if (ed->cursor_col > (pl->len+(pl->indent*INDENT_SPACES)))
    {
        ed->cursor_col = pl->len+(pl->indent*INDENT_SPACES);
    }
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool cursorDown(IDE_instance ed)
{
    if (ed->editing)
        commitBuf (ed);
    IDE_line nl = ed->cursor_line;
    if (nl->folded)
    {
        do
        {
            nl = nl->next;
        } while (nl && !nl->fold_end);
        if (nl)
            nl = nl->next;
    }
    else
    {
        nl = nl->next;
    }
    if (!nl)
        return FALSE;

    ed->cursor_line = nl;
    ed->cursor_a_line = nl->a_line;
    ed->cursor_v_line = nl->v_line;
    if (ed->cursor_col > (nl->len+(nl->indent*INDENT_SPACES)))
        ed->cursor_col = nl->len+(nl->indent*INDENT_SPACES);
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool cursorLeft(IDE_instance ed)
{
    if (ed->cursor_col == 0)
        return FALSE;

    ed->cursor_col--;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool cursorRight(IDE_instance ed)
{
    int len = ed->editing ? ed->buf_len : ed->cursor_line->len + INDENT_SPACES * ed->cursor_line->indent;
    if (ed->cursor_line->folded)
        len += 1;
    if (ed->cursor_col >= len)
        return FALSE;

    ed->cursor_col++;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool pageUp(IDE_instance ed)
{
    for (uint16_t i = 0; i<ed->window_height; i++)
    {
        if (!cursorUp(ed))
            return FALSE;
    }
    return TRUE;
}

static bool pageDown(IDE_instance ed)
{
    for (uint16_t i = 0; i<ed->window_height; i++)
    {
        if (!cursorDown(ed))
            return FALSE;
    }
    return TRUE;
}

static bool gotoBOF(IDE_instance ed)
{
    if (ed->editing)
        commitBuf (ed);

    ed->cursor_line = ed->line_first;

    ed->cursor_a_line = 0;
    ed->cursor_v_line = 0;
    ed->cursor_col = 0;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool gotoEOF(IDE_instance ed)
{
    if (ed->editing)
        commitBuf (ed);

    ed->cursor_line = ed->line_first;
	while (ed->cursor_line->next)
	{
		ed->cursor_line = ed->cursor_line->next;
	}

    ed->cursor_a_line = ed->cursor_line->a_line;
    ed->cursor_v_line = ed->cursor_line->v_line;
    ed->cursor_col = 0;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static bool gotoHome(IDE_instance ed)
{
    ed->cursor_col = 0;
    ed->up2date_il_pos = FALSE;
    return TRUE;
}

static bool gotoEnd(IDE_instance ed)
{
    if (ed->editing)
        ed->cursor_col = ed->buf_len;
    else
        ed->cursor_col = ed->cursor_line->len + ed->cursor_line->indent * INDENT_SPACES;
    ed->up2date_il_pos = FALSE;
    return TRUE;
}

static void fold (IDE_instance ed, IDE_line cl)
{
    if (!cl->fold_start)
        return;

    if (ed->editing)
    {
        bool clIsCursorLine = cl == ed->cursor_line;
        IDE_line cl2 = commitBuf (ed);
        if (clIsCursorLine)
            cl = cl2;
    }

    IDE_line fl = cl; // remember fold start

    do
    {
        cl->folded = !cl->folded;
        cl = cl->next;
    } while (cl && !cl->fold_end);

    if (cl)
        cl->folded = !cl->folded;

    indentSuccLines (ed, fl);
    invalidateAll(ed);
}

static void unfoldLine (IDE_instance ed, IDE_line l)
{
    if (l->folded)
    {
        IDE_line fs = l;
        while (fs && !fs->fold_start)
            fs = fs->prev;
        if (fs)
            fold(ed, fs);
    }
}

static void unfoldCursorLine(IDE_instance ed)
{
    unfoldLine (ed, ed->cursor_line);
}

static bool gotoLine(IDE_instance ed, uint16_t line, uint16_t col)
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

    unfoldCursorLine(ed);

    ed->cursor_a_line = ed->cursor_line->a_line;
    ed->cursor_v_line = ed->cursor_line->v_line;
    ed->cursor_col = col-1;
    ed->up2date_il_pos = FALSE;

    return TRUE;
}

static void line2buf (IDE_instance ed, IDE_line l)
{
    if (l->folded)
        fold(ed, l);

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

static void repaintLine (IDE_instance ed, char *buf, char *style, uint16_t len, uint16_t row, uint16_t indent, bool folded)
{
    // FIXME: horizontal scroll

    UI_beginLine (row, 1, UI_size_cols);

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

static void repaint (IDE_instance ed)
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
    uint16_t row = 1;
    while (l && (linenum <= linenum_end))
    {
        if (!ed->up2date_row[row-1])
        {
            if (ed->editing && (linenum == ed->cursor_v_line))
                repaintLine (ed, ed->buf, ed->style, ed->buf_len, row, 0, /*folded=*/FALSE);
            else
                repaintLine (ed, l->buf, l->style, l->len, row, l->indent, l->folded);
            ed->up2date_row[row-1] = TRUE;
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
        while (row <= ed->infoline_row)
        {
            UI_beginLine (row, 1, UI_size_cols);
            UI_endLine   ();
            row++;
        }
    }

    // infoline

    bool update_infoline = FALSE;
    if (!ed->up2date_il_pos)
    {
        _itoa (ed->cursor_col+1, ed->infoline + INFOLINE_CURSOR_X, 4);
        _itoa (ed->cursor_a_line+1, ed->infoline + INFOLINE_CURSOR_Y, 4);
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
        UI_beginLine (ed->infoline_row+1, 1, UI_size_cols);
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

    UI_moveCursor (ed->cursor_v_line-ed->scrolloff_row+1, ed->cursor_col-ed->scrolloff_col+1);
    UI_setCursorVisible (TRUE);

#ifdef ENABLE_REPAINT_BENCHMARK
    float stopTime = U_getTime();
    printf ("IDE: repaint() took %d-%d = %d ms\n", (int)stopTime, (int)startTime, (int) (1000.0 * (stopTime-startTime)));
#endif
}

static void enterKey (IDE_instance ed)
{
    // split line ?
    uint16_t l = ed->editing ? ed->buf_len : ed->cursor_line->len + ed->cursor_line->indent*INDENT_SPACES;
    if (ed->cursor_line->folded)
        l++;
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
        while (ed->cursor_line->folded && ed->cursor_line->next && ed->cursor_line->next->folded)
            ed->cursor_line = ed->cursor_line->next;
    }

    IDE_line line = buf2line (ed);
    insertLineAfter (ed, ed->cursor_line, line);
    indentSuccLines (ed, ed->cursor_line);
    ed->cursor_col = 0;
    ed->cursor_line = line;
    ed->cursor_a_line = ed->cursor_line->a_line;
    ed->cursor_v_line = ed->cursor_line->v_line;
    ed->num_lines++;
    invalidateAll(ed);
}

static void backspaceKey (IDE_instance ed)
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
        unfoldLine (ed, pl);
        unfoldLine (ed, cl);
        line2buf (ed, pl);
        ed->cursor_col = ed->buf_len;
        ed->cursor_line = pl;
        ed->cursor_a_line = ed->cursor_line->a_line;
        ed->cursor_v_line = ed->cursor_line->v_line;
        ed->buf[ed->buf_len] = ' ';
        ed->style[ed->buf_len] = UI_TEXT_STYLE_TEXT;
        ed->buf_len++;
        memcpy (ed->buf+ed->buf_len,   cl->buf  , cl->len);
        memcpy (ed->style+ed->buf_len, cl->style, cl->len);
        ed->buf_len += cl->len;
        ed->editing = TRUE;
        deleteLine (ed, cl);
        ed->cursor_line = commitBuf (ed);
        indentSuccLines (ed, ed->cursor_line);
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
        ed->up2date_row[ed->cursor_v_line - ed->scrolloff_row] = FALSE;
        cursorLeft(ed);
    }
}

static void deleteKey (IDE_instance ed)
{
    // join lines ?
    uint16_t l = ed->editing ? ed->buf_len : ed->cursor_line->len + ed->cursor_line->indent*INDENT_SPACES;
    if (ed->cursor_col == l)
    {
        if (!ed->cursor_line->next)
            return;
        IDE_line cl = ed->cursor_line;
        unfoldLine(ed, cl);
        unfoldLine(ed, cl->next);

        if (!ed->editing)
            line2buf (ed, ed->cursor_line);

        IDE_line nl = cl->next;
        memcpy (ed->buf+ed->buf_len,   nl->buf  , nl->len);
        memcpy (ed->style+ed->buf_len, nl->style, nl->len);
        ed->buf_len += nl->len;
        deleteLine (ed, nl);
        ed->cursor_line = commitBuf (ed);
        ed->num_lines--;
        indentSuccLines (ed, ed->cursor_line);
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
        ed->up2date_row[ed->cursor_v_line - ed->scrolloff_row] = FALSE;
    }
}


static void killLine (IDE_instance ed)
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

    unfoldCursorLine(ed);
    if (ed->cursor_col > nl->len)
        ed->cursor_col = nl->len;

    ed->cursor_line = nl;
    //if (nl->folded)
    //    fold (ed, nl);

    deleteLine (ed, cl);
    indentSuccLines (ed, nl->prev ? nl->prev : nl);
    invalidateAll(ed);
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
static bool insertChar (IDE_instance ed, uint16_t c)
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

    ed->up2date_row[ed->cursor_v_line - ed->scrolloff_row] = FALSE;

    cursorRight(ed);
    //LOG_printf (LOG_DEBUG, "insertChar %c 4\n", c);

    return TRUE;
}

static void IDE_setSourceFn(IDE_instance ed, string sourcefn)
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

        string module_name = basename(String(UP_ide, sourcefn));
        l = strlen(module_name);
        if (l>PATH_MAX)
            l = PATH_MAX;
        if (l>4)
            module_name[l-4] = 0;
        strncpy (ed->module_name, module_name, PATH_MAX);

        OPT_addModulePath(dirname(String(UP_ide, sourcefn)));
    }
    else
    {
        ed->sourcefn[0] = 0;
        ed->binfn[0]    = 0;
    }
    ed->up2date_il_sourcefn=FALSE;
}

#ifdef __amigaos__
#pragma GCC pop_options
#endif
static bool IDE_save (IDE_instance ed, bool save_as)
{
    if (ed->editing)
        commitBuf (ed);

    if (!strlen(ed->sourcefn) || save_as)
    {
        char *sourcefn = UI_FileReq ("Save BASIC source code as...");

        if (!sourcefn)
            return FALSE;

        IDE_setSourceFn(ed, sourcefn);
    }

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

#ifdef __amigaos__
	PutDiskObject((STRPTR)ed->sourcefn, &aqbsrcIcon);
#endif

    ed->changed = FALSE;
    ed->up2date_il_flags = FALSE;

    return TRUE;
}

static void IDE_exit (IDE_instance ed)
{
    LOG_printf (LOG_DEBUG, "ide: IDE_exit\n");
    if (ed->changed)
    {
        if (UI_EZRequest ("Save changes to disk?", "Yes|No"))
            IDE_save(ed, /*save_as=*/FALSE);
    }
    UI_setCursorVisible(TRUE);
    LOG_printf (LOG_DEBUG, "ide: IDE_exit -> exit(0)\n");
    exit(0);
}

static void show_help(IDE_instance ed)
{
#ifdef __amigaos__
    UI_HelpBrowser();
#else
    // FIXME UI_HelpBrowser();
    UI_EZRequest ("AQB Amiga QuickBasic Compiler IDE\n\nKeyboard shortcuts:\n\n"
                  "F1     - this help screen\n"
                  "S-UP   - page up\n"
                  "S-DOWN - page down\n"
                  "Ctrl-T - goto top of file\n"
                  "Ctrl-B - goto end of file\n"
                  "Ctrl-Y - delete line\n"
                  "F5     - compile & run\n"
                  "F7     - compile\n"
                  "Ctrl-F - find\n"
                  "Ctrl-N - find next\n"
                  "Ctrl-M - mark block\n"
                  "Ctrl-S - save\n"
                  "Ctrl-C - quit", "Close");
    invalidateAll (ed);
#endif
}

static void show_about(IDE_instance ed)
{
    UI_EZRequest (PROGRAM_NAME_LONG " " VERSION"\n\n"
                  COPYRIGHT "\n\n"
                  LICENSE, "Close");
    invalidateAll (ed);
}

static bool compile(IDE_instance ed)
{
    if (!IDE_save(ed, /*save_as=*/FALSE))
        return FALSE;

    OPT_reset();

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

    UI_eraseDisplay ();
    invalidateAll (ed);

    if (EM_anyErrors)
    {
        gotoLine (ed, EM_firstErrorLine, EM_firstErrorCol);
        ed->il_show_error = TRUE;
        return FALSE;
    }
    else
    {
#ifdef __amigaos__
        PutDiskObject((STRPTR)ed->binfn, &aqbbinIcon);
#endif
    }
    return TRUE;
}

static void compileAndRun(IDE_instance ed)
{
#ifdef __amigaos__
    if (RUN_getState() != RUN_stateStopped)
    {
        RUN_break();
        return;
    }
#endif

    bool changed = ed->changed || !strlen (ed->sourcefn) || !strlen(ed->binfn);
    if (!changed)
    {
        //printf ("!changed sourcefn=%s binfn=%s\n", ed->sourcefn, ed->binfn);
        struct stat srcstat, binstat;
        if (stat (ed->sourcefn, &srcstat))
        {
            //printf ("!stat\n");
            changed = TRUE;
        }
        else
        {
            if (stat (ed->binfn, &binstat))
            {
                //printf ("!stat bin\n");
                changed = TRUE;
            }
            else
            {
                //printf ("!stat mtime: binstat.st_mtime=%ld <? srcstat.st_mtime=%ld\n", binstat.st_mtime, srcstat.st_mtime);
                changed = binstat.st_mtime < srcstat.st_mtime;
            }
        }
    }

    if (changed)
    {
        if (!compile(ed))
            return;
    }

#ifdef __amigaos__

    UI_setCursorVisible (FALSE);
    RUN_start (ed->binfn);
    // RUN_start ("SYS:Utilities/Clock"); // debug purposes only

#else

    LOG_printf (LOG_INFO, "\n*** FIXME: non-amiga debugging not implemented yet.\n\n");
    LOG_printf (LOG_INFO, "\n*** press enter to continue ***\n\n");
    while ( UI_waitkey () != KEY_ENTER ) ;
    UI_eraseDisplay ();
    invalidateAll (ed);
    UI_setCursorVisible (TRUE);

#endif

}

static bool isWhitespace (char c)
{
    switch (c)
    {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\v':
        case '\f':
        case 0:
            return TRUE;
    }
    return FALSE;
}

static int16_t _strstr (const char *s, int16_t col, const char *find, bool match_case, bool whole_word, bool backwards)
{
    // first character of search string

    char c;
    if ((c = *find++) == 0)
        return -1;
    c = match_case ? c : tolower((unsigned char)c);

    int16_t len    = strlen(find);
    int16_t maxcol = strlen(s) - len;

    if (col > maxcol)
    {
        if (backwards)
            col = maxcol;
        else
            return -1;
    }

    while (TRUE)
    {
        // first character matches ?
        char sc = s[col];
        if ( (match_case && (sc!=c))
              || (!match_case && (char)tolower((unsigned char)sc) != c))
              goto nextcol;

        // does the rest match?
        if ( (match_case && strncmp (s+col+1, find, len)) || (!match_case && strncasecmp(s+col+1, find, len)) )
            goto nextcol;

        if (whole_word)
        {
            if (col != 0) // BOL is a word boundary
            {
                char sc = s[col-1];
                if (!isWhitespace(sc))
                    goto nextcol;
            }

            if (!isWhitespace (s[col+len+1]))
                goto nextcol;
        }

        return col;

nextcol:
        col = backwards ? col - 1 : col + 1;
        if ( (col > maxcol) || (col < 0) )
            return -1;
    };

    return -1;
}


static void findNext (IDE_instance ed, bool first)
{
	if (!strlen(ed->find_buf))
		return;

    if (ed->editing)
        commitBuf (ed);

    int16_t col = ed->cursor_col;
    IDE_line l = ed->cursor_line;

    if (!first)
    {
        if (ed->find_searchBackwards)
        {
            if (col>0)
            {
                col--;
            }
            else
            {
                l = l->prev;
                if (!l)
                    return;
                col = strlen(l->buf);
            }
        }
		else
        {
            col++;
        }
    }

    bool found = FALSE;
    while (!found && l)
    {
        LOG_printf (LOG_DEBUG, "findNext: looking for %s in %s\n", ed->find_buf, l->buf);
        col = _strstr (l->buf, col, ed->find_buf, ed->find_matchCase, ed->find_wholeWord, ed->find_searchBackwards);
        if (col >= 0)
        {
            found = TRUE;
            ed->cursor_col = col + l->indent * INDENT_SPACES;
        }
        else
        {
            if (!ed->find_searchBackwards)
            {
                l = l->next;
                col=0;
            }
            else
            {
                l = l->prev;
                if (!l)
                    return;
                col = strlen(l->buf);
            }
        }
    }

    if (found)
    {
        ed->cursor_line = l;
        unfoldCursorLine(ed);
        ed->cursor_a_line = ed->cursor_line->a_line;
        ed->cursor_v_line = ed->cursor_line->v_line;
        ed->up2date_il_pos = FALSE;
    }
}

static void IDE_find (IDE_instance ed)
{
    if (UI_FindReq (ed->find_buf, MAX_LINE_LEN, &ed->find_matchCase, &ed->find_wholeWord, &ed->find_searchBackwards))
        findNext (ed, /*first=*/TRUE);

    UI_eraseDisplay ();
    invalidateAll (ed);
    UI_setCursorVisible (TRUE);
}

static void doClear (IDE_instance ed)
{
    IDE_line l=ed->line_first;
    while (l)
    {
        IDE_line nl = l->next;
        freeLine(ed, l);
        l = nl;
    }
    ed->line_first     = NULL;
    ed->line_last      = NULL;
    ed->cursor_line    = NULL;
    ed->sourcefn[0]    = 0;
    ed->module_name[0] = 0;
}

static void doNew (IDE_instance ed)
{
    doClear(ed);
    ed->buf_len = 0;
    ed->buf[ed->buf_len] = 0;
    IDE_line line = buf2line (ed);
    insertLineAfter (ed, NULL, line);
}

static void loadSource (IDE_instance ed, string sourcefn)
{
    doClear (ed);
    IDE_setSourceFn(ed, sourcefn);

    if (sourcefn)
    {
        FILE *sourcef = fopen(sourcefn, "r");
        if (sourcef)
        {
            ed->buf_len = 0;
            bool eof = FALSE;
            bool eol = FALSE;
            IDE_line lastLine = NULL;
            bool folded = FALSE;
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
                    eol = ed->buf_len>0;
                }

                if (eol)
                {
                    ed->buf[ed->buf_len] = 0;
                    IDE_line line = buf2line (ed);
                    if (line->fold_start)
                    {
                        line->folded = TRUE;
                        folded = TRUE;
                    }
                    else
                    {
                        if (line->fold_end)
                        {
                            line->folded = TRUE;
                            folded = FALSE;
                        }
                        else
                        {
                            line->folded = folded;
                        }
                    }
                    insertLineAfter (ed, lastLine, line);
                    lastLine = line;
                    eol=FALSE;
                    ed->buf_len = 0;
                }
            }

            fclose(sourcef);
        }
        else
        {
            UI_EZRequest("failed to read %s:\n%s", "OK", sourcefn, strerror(errno));
            doNew(ed);
        }
    }
    else
    {
        doNew(ed);
    }

    ed->cursor_col		 = 0;
    ed->cursor_a_line	 = 0;
    ed->cursor_v_line	 = 0;
    ed->cursor_line      = ed->line_first;

    indentSuccLines (ed, ed->line_first);
    invalidateAll (ed);
}

static void IDE_new (IDE_instance ed)
{
    if (ed->editing)
        commitBuf (ed);
    if (ed->changed)
    {
        if (UI_EZRequest ("Save changes to disk?", "Yes|No"))
            IDE_save(ed, /*save_as=*/FALSE);
    }

    doNew(ed);

    ed->cursor_col		 = 0;
    ed->cursor_a_line	 = 0;
    ed->cursor_v_line	 = 0;
    ed->cursor_line      = ed->line_first;

    indentSuccLines (ed, ed->line_first);
    invalidateAll (ed);
}

static void IDE_load_FileReq (IDE_instance ed)
{
    if (ed->editing)
        commitBuf (ed);
    if (ed->changed)
    {
        if (UI_EZRequest ("Save changes to disk?", "Yes|No"))
            IDE_save(ed, /*save_as=*/FALSE);
    }

    char *sourcefn = UI_FileReq ("Load BASIC source code file");
    if (sourcefn)
        loadSource (ed, sourcefn);
}

static void size_cb (void *user_data)
{
	IDE_instance ed = (IDE_instance) user_data;
    initWindowSize (ed);
    invalidateAll (ed);
    scroll(ed);
    UI_eraseDisplay();
    repaint(ed);
}

static void handleRunStop(IDE_instance ed)
{
#ifdef __amigaos__
    LOG_printf (LOG_INFO, "\n\nPROGRAM EXITED\n\n");
    LOG_printf (LOG_INFO, "\n*** PRESS ANY KEY TO CONTINUE ***\n\n");
    UI_waitkey ();
#endif

    UI_eraseDisplay ();
    invalidateAll (ed);
    UI_setCursorVisible (TRUE);
}

static void event_cb (uint16_t key, void *user_data)
{
	IDE_instance ed = (IDE_instance) user_data;

    // are we in debug mode right now ?

    RUN_state rs = RUN_getState();
    if (rs != RUN_stateStopped)
    {
        RUN_handleEvent (key);
        rs = RUN_getState();
        if (rs == RUN_stateStopped)
        {
            UI_eraseDisplay ();
            invalidateAll (ed);
            UI_setCursorVisible (TRUE);
        }
        return;
    }

    switch (key)
    {
        case KEY_CTRL_C:
        case KEY_CTRL_Q:
        case KEY_QUIT:
            IDE_exit(ed);
            break;

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

        case KEY_NEW:
            IDE_new(ed);
            break;

        case KEY_CTRL_O:
        case KEY_OPEN:
            IDE_load_FileReq(ed);
            break;

        case KEY_CTRL_S:
        case KEY_SAVE:
            IDE_save(ed, /*save_as=*/FALSE);
            break;

        case KEY_SAVE_AS:
            IDE_save(ed, /*save_as=*/TRUE);
            break;

        case KEY_HELP:
        case KEY_F1:
            show_help(ed);
            break;

        case KEY_ABOUT:
            show_about(ed);
            break;

        case KEY_F5:
            compileAndRun(ed);
            break;

        case KEY_STOPPED:
            handleRunStop(ed);
            break;

        case KEY_CTRL_A:
        case KEY_CTRL_F:
        case KEY_FIND:
            IDE_find(ed);
            break;

		case KEY_FIND_NEXT:
		case KEY_CTRL_N:
			findNext (ed, /*first=*/FALSE);
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

		case KEY_FONT_0:
		case KEY_FONT_1:
			UI_setFont (key - KEY_FONT_0);
            initWindowSize (ed);
            invalidateAll (ed);
			break;

        case KEY_NONE:
            break;

        case KEY_TAB:
            fold (ed, ed->cursor_line);
            break;

        default:
            if (!insertChar(ed, (uint8_t) key))
                UI_bell();
            break;

    }

    scroll(ed);
    repaint(ed);
}

IDE_instance openEditor(void)
{
	IDE_instance ed = U_poolAlloc (UP_ide, sizeof (*ed));

    strncpy (ed->infoline, INFOLINE, sizeof(ed->infoline));
    ed->infoline_row         = 0;
    ed->sourcefn[0]          = 0;
    ed->module_name[0]       = 0;
    ed->line_first           = NULL;
    ed->line_last            = NULL;
    ed->num_lines	         = 0;
    ed->num_vlines	         = 0;
    ed->scrolloff_col        = 0;
    ed->scrolloff_row        = 0;
    ed->scrolloff_line       = NULL;
    ed->cursor_col		     = 0;
    ed->cursor_a_line	     = 0;
    ed->cursor_v_line	     = 0;
    ed->cursor_line          = NULL;
    ed->changed		         = FALSE;
    ed->editing              = FALSE;
    ed->il_show_error        = FALSE;
    ed->find_buf[0]          = 0;
    ed->find_matchCase       = FALSE;
    ed->find_wholeWord       = FALSE;
    ed->find_searchBackwards = FALSE;

    invalidateAll (ed);

    initWindowSize(ed);

	return ed;
}

static void log_cb (uint8_t lvl, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    static char buf[1024];
    vsnprintf (buf, 1024, fmt, args);
    va_end(args);
	if (lvl >= LOG_INFO)
        UI_tprintf ("%s", buf);
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
#if LOG_LEVEL == LOG_DEBUG
    //printf ("opening %s ...\n", LOG_FILENAME);
    logf = fopen (LOG_FILENAME, "a");
    //printf ("opening %s ... done.\n", LOG_FILENAME);
#endif
	atexit (IDE_deinit);
    UI_init();
#ifdef __amigaos__
    RUN_init(UI_debugPort());
#endif
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
    S_REM      = S_Symbol ("REM"     , FALSE);

    g_keywords = TAB_empty (UP_ide);
    for (int i =0; i<FE_num_keywords; i++)
        TAB_enter (g_keywords, FE_keywords[i], (void *) TRUE);

    g_ed = openEditor();

    loadSource (g_ed, sourcefn);

    UI_setCursorVisible (FALSE);
    UI_moveCursor (1, 1);
    UI_eraseDisplay();
    repaint(g_ed);

	UI_onEventCall(event_cb, g_ed);
    UI_onSizeChangeCall (size_cb, g_ed);

	UI_run();
}

