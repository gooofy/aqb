#ifndef HAVE_TERMINAL_H
#define HAVE_TERMINAL_H

#include "util.h"

#define TE_MIN_COLUMNS     10
#define TE_MAX_COLUMNS     256
#define TE_MIN_ROWS        5
#define TE_MAX_ROWS        256

#define KEY_ESC            27
#define KEY_CTRL_C          3
#define KEY_CTRL_D          4
#define KEY_CTRL_F          6
#define KEY_CTRL_G          7
#define KEY_CTRL_H          8
#define KEY_TAB             9
#define KEY_CTRL_L         12
#define KEY_ENTER          13
#define KEY_CTRL_Q         17
#define KEY_CTRL_S         19
#define KEY_CTRL_U         21
#define KEY_BACKSPACE     127
#define KEY_CURSOR_LEFT  1000
#define KEY_CURSOR_RIGHT 1001
#define KEY_CURSOR_UP    1002
#define KEY_CURSOR_DOWN  1003
#define KEY_DEL          1004
#define KEY_HOME         1005
#define KEY_END          1006
#define KEY_PAGE_UP      1007
#define KEY_PAGE_DOWN    1008

void  TE_flush            (void);
void  TE_putc             (char c);
void  TE_putstr           (string s);
void  TE_printf           (char* format, ...);
int   TE_getch            (void);
void  TE_bell             (void);

void  TE_moveCursor       (int row, int col);
void  TE_eraseToEOL       (void);
void  TE_eraseDisplay     (void);
void  TE_setCursorVisible (bool visible);

#define TE_STYLE_NORMAL     0
#define TE_STYLE_BOLD       1
#define TE_STYLE_ITALICS    3
#define TE_STYLE_UNDERLINE  4
#define TE_STYLE_INVERSE    7

void  TE_setTextStyle     (int style);

bool  TE_init             (void);

bool  TE_getsize          (int *rows, int *cols);
void  TE_onSizeChangeCall (void (*cb)(void));

float TE_get_time        (void);

#endif

