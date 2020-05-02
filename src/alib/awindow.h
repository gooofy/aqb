#ifndef HAVE_AWINDOW_H
#define HAVE_AWINDOW_H

#include "autil.h"

#define AW_FLAG_SIZE     1
#define AW_FLAG_DRAG     2
#define AW_FLAG_DEPTH    4
#define AW_FLAG_CLOSE    8
#define AW_FLAG_REFRESH 16

void _awindow_init(void);

BOOL __aqb_window_open(short id, char *title, short x1, short y1, short x2, short y2, short flags, short scr_id);

void _awindow_shutdown(void);

#define AW_LINE_STEP_1    1
#define AW_LINE_STEP_2    2
#define AW_LINE_FLAG_BOX  4
#define AW_LINE_FLAG_FILL 8

BOOL __aqb_line(short x1, short y1, short x2, short y2, short flags, short color);

void __aqb_sleep(void);

void __aqb_on_window_call(void (*cb)(void));

ULONG __aqb_window_fn(short n);

#define AW_PSET_STEP  1
#define AW_PSET_RESET 2

BOOL __aqb_pset(short x, short y, short flags, short color);

#endif

