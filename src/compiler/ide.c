#include "ide.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "util.h"
#include "terminal.h"
#include "scanner.h"

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
	IDE_line next, prev;
	uint8_t  len;
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
	uint16_t           scrolloff_col, scrolloff_row;
	uint16_t           cursor_col, cursor_row;
	bool               changed;
	bool               insert;
	bool               tabs;
	bool               skipblanks;
	bool               autoindent;
	uint16_t           window_width, window_height;
	char               infoline[36];
    uint16_t           infoline_row;
	uint8_t	           filename[PATH_MAX];
};

IDE_line newLine(IDE_editor ed, uint16_t len)
{
    IDE_line l = U_poolAlloc (UP_ide, sizeof(*l)+len);

	l->next  = NULL;
	l->prev  = NULL;
	l->len   = 0;

	return l;
}

#if 0
static void IDE_lineListInsertAfter (IDE_lineList *ll, IDE_line lBefore, IDE_line l)
{
    if (lBefore)
    {
        assert(FALSE); // FIXME: implement
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
#endif
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
    TE_flush        ();
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
    TE_moveCursor (0, 0);
    TE_eraseDisplay();
    showInfoLine(ed);

	return ed;
}

static void IDE_exit(void)
{
    TE_moveCursor (0, 0);
    TE_eraseDisplay();
    TE_flush();
}

static void IDE_load (char *sourcefn)
{
	FILE *sourcef = fopen(sourcefn, "r");
	if (!sourcef)
	{
		fprintf(stderr, "failed to read %s: %s\n\n", sourcefn, strerror(errno));
		exit(2);
	}
    //S_init (sourcef);
    fclose(sourcef);
}

void IDE_open(char *fn)
{
    TE_init();
    TE_setCursorVisible (TRUE);

    atexit (IDE_exit);

	IDE_editor ed = OpenEditor();

    if (fn)
        IDE_load (fn);

    // FIXME showCursor(ed);
    bool running = TRUE;
    while (running)
    {
        int ch = TE_getch();
        running = handleKey(ed, ch);
    }
}

