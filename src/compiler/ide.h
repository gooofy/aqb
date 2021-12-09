#ifndef HAVE_IDE_H
#define HAVE_IDE_H

#include <limits.h>
#include <inttypes.h>

#include "util.h"
#include "ui.h"
#include "scanner.h"

#define INDENT_SPACES 4

#define MAX_CON_LINES     64
#define MAX_CON_LINE_LEN  80

typedef enum {DEBUG_stateStopped, DEBUG_stateRunning, DEBUG_stateTrapped} DEBUG_state;

typedef struct IDE_line_     *IDE_line;
typedef struct IDE_instance_ *IDE_instance;

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
    uint16_t  a_line; // absolut line number in file
    uint16_t  v_line; // virtual line number, takes folding into account

    // hilighting
    uint16_t  h_start, h_end;

    bool      up2date; // false -> needs redraw if visible on screen
};

struct IDE_instance_
{
    UI_view            view_editor, view_status;

    IDE_line           line_first, line_last;
	uint16_t           num_lines;
    uint16_t           num_vlines;
	bool               changed;
	char               infoline[36+PATH_MAX];
	char 	           sourcefn[PATH_MAX];
	char 	           module_name[PATH_MAX];
	char 	           binfn[PATH_MAX];

	int16_t            cursor_col, cursor_a_line, cursor_v_line;
    IDE_line           cursor_line;

    // window, scrolling, repaint
	int16_t            window_width, window_height;
	int16_t            scrolloff_col, scrolloff_row;
    IDE_line           scrolloff_line;
    int16_t            scrolloff_line_row;
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
    // buf2line support:
    uint16_t           buf_pos;
    char               buf_ch;
    bool               eol;

    // temporary buffer used in edit operations
    char               buf2[MAX_LINE_LEN];
    char               style2[MAX_LINE_LEN];
    uint16_t           buf2_len;

    // find/replace
    bool               find_matchCase, find_wholeWord, find_searchBackwards;
    char               find_buf[MAX_LINE_LEN];

    // console view
    UI_view            view_console;
    int16_t            con_rows, con_cols;                          // current size of console view
    char               con_buf  [MAX_CON_LINES][MAX_CON_LINE_LEN];
    char               con_style[MAX_CON_LINES][MAX_CON_LINE_LEN];
    int16_t            con_line, con_col;                           // curent caret line/col in console buffer
};

/*
 * IDE
 */

void     IDE_open         (string sourcefn);

void     IDE_gotoLine     (IDE_instance ed, uint16_t line, uint16_t col, bool hilight);
void     IDE_clearHilight (IDE_instance ed);

/*
 * console view (simple terminal emulation)
 */

void     IDE_conInit   (IDE_instance ed);
void     IDE_conSet    (IDE_instance ed, bool visible, bool active);
void     IDE_cprintf   (IDE_instance ed, char* format, ...);
void     IDE_cvprintf  (IDE_instance ed, char* format, va_list ap);
void     IDE_readline  (IDE_instance ed, char *buf, int16_t buf_len);

/*
 * debugger
 */

DEBUG_state DEBUG_getState(void);

#ifdef __amigaos__

#include <dos/dosextens.h>

bool      DEBUG_start          (const char *binfn);
void      DEBUG_help           (char *binfn, char *arg1);

void      DEBUG_init           (IDE_instance ide, struct MsgPort *debugPort);

uint16_t  DEBUG_handleMessages (void);  // called by ui_amiga when message arrive at our debugPort

void      DEBUG_break          (bool waitForTermination);  // send SIGBREAKF_CTRL_C to child

void      DEBUG_disasm         (IDE_instance ed, unsigned long int start, unsigned long int end);

#endif

#endif

