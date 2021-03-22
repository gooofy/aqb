#ifndef HAVE_TERMINAL_H
#define HAVE_TERMINAL_H

#include "util.h"

#define TE_MIN_COLUMNS     10
#define TE_MAX_COLUMNS     256
#define TE_MIN_ROWS        5
#define TE_MAX_ROWS        256

void  TE_flush            (void);
void  TE_putc             (char c);
void  TE_putstr           (string s);
void  TE_printf           (char* format, ...);
int   TE_getch            (void);

void  TE_moveCursor       (int row, int col);
void  TE_eraseToEOL       (void);
void  TE_setCursorVisible (bool visible);

#define TE_STYLE_NORMAL     0
#define TE_STYLE_BOLD       1
#define TE_STYLE_ITALICS    3
#define TE_STYLE_UNDERLINE  4
#define TE_STYLE_INVERSE    7

void  TE_setTextStyle     (int style);

void  TE_init             (void);
void  TE_exit             (int return_code);

bool  TE_getsize          (int *rows, int *cols);
void  TE_onSizeChangeCall (void (*cb)(void));

float TE_get_time        (void);

#endif

