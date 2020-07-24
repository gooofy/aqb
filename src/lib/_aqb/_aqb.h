#ifndef HAVE_AQB_H
#define HAVE_AQB_H

#include "../_brt/_brt.h"

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase       *GfxBase;

/*
 * print statement support
 */

extern BPTR g_stdout;

void _aio_init(void);
void _aio_shutdown(void);

void _aio_puts4(int num);
void _aio_puts2(short num);
void _aio_puts1(char num);
void _aio_putu4(unsigned int num);
void _aio_putu2(unsigned short num);
void _aio_putu1(unsigned char num);
void _aio_puthex(int num);
void _aio_putuhex(ULONG l);
void _aio_putbin(int num);
void _aio_putf(FLOAT f);
void _aio_putbool(BOOL b);

void _aio_puts(const char *str);

void _aio_putnl(void);
void _aio_puttab(void);

void  LOCATE  (short l, short c);
short CSRLIN_ (void);
short POS_    (short dummy);

/*
 * screens, windows, graphics
 */

#define AS_MODE_LORES                1
#define AS_MODE_HIRES                2
#define AS_MODE_LORES_LACED          3
#define AS_MODE_HIRES_LACED          4
#define AS_MODE_HAM                  5
#define AS_MODE_EXTRAHALFBRITE       6
#define AS_MODE_HAM_LACED            7
#define AS_MODE_EXTRAHALFBRITE_LACED 8

#define AW_FLAG_SIZE                 1
#define AW_FLAG_DRAG                 2
#define AW_FLAG_DEPTH                4
#define AW_FLAG_CLOSE                8
#define AW_FLAG_REFRESH             16

#define AE_WIN_OPEN                  1
#define AE_SCREEN_OPEN               2

void _awindow_init(void);
void _awindow_shutdown(void);

void   SCREEN (short id, short width, short height, short depth, short mode);
void   WINDOW(short id, char *title, BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short flags, short scrid);
void   LINE(BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short c, short bf);
void   SLEEP(void);
void   ON_WINDOW_CALL(void (*cb)(void));
ULONG  WINDOW_(short n);
void   PSET(BOOL s, short x, short y, short color);
char  *INKEY_ (void);

#endif

