#ifndef HAVE_IDE_H
#define HAVE_IDE_H

#include <limits.h>
#include <inttypes.h>

#include "util.h"
#include "ui.h"
#include "scanner.h"

#define INDENT_SPACES 4

typedef enum {DEBUG_stateStopped, DEBUG_stateRunning} DEBUG_state;

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
};

struct IDE_instance_
{
    IDE_line           line_first, line_last;
	uint16_t           num_lines;
    uint16_t           num_vlines;
	bool               changed;
	char               infoline[36+PATH_MAX];
    uint16_t           infoline_row;
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
};

/*
 * IDE
 */

void     IDE_open (string sourcefn);

IDE_line IDE_getVLine (IDE_instance ed, int linenum);   // virtual line -> folded lines do not count
IDE_line IDE_getALine (IDE_instance ed, int linenum);   // actual line -> raw line numbers not taking folding into account

/*
 * debugger
 */

DEBUG_state DEBUG_getState(void);

#ifdef __amigaos__

#include <dos/dosextens.h>

void      DEBUG_start          (const char *binfn);
void      DEBUG_help           (char *binfn, char *arg1);

void      DEBUG_init           (IDE_instance ide, struct MsgPort *debugPort);

uint16_t  DEBUG_handleMessages (void);  // called by ui_amiga when message arrive at our debugPort

void      DEBUG_break          (void);  // send SIGBREAKF_CTRL_C to child

#endif

#endif

