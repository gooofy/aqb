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

// [ LINE ] INPUT support:

void _aio_gets                   (char **s, BOOL do_nl);
void _aio_line_input             (char *prompt, char **s, BOOL do_nl);
void _aio_console_input          (BOOL qm, char *prompt, BOOL do_nl);
void _aio_inputs1                (BYTE   *v);
void _aio_inputu1                (UBYTE  *v);
void _aio_inputs2                (SHORT  *v);
void _aio_inputu2                (USHORT *v);
void _aio_inputs4                (LONG   *v);
void _aio_inputu4                (ULONG  *v);
void _aio_inputf                 (FLOAT  *v);
void _aio_inputs                 (char  **v);
void _aio_set_dos_cursor_visible (BOOL visible);

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
#define AW_FLAG_BACKDROP            32
#define AW_FLAG_BORDERLESS          64

#define AE_WIN_OPEN                 101
#define AE_SCREEN_OPEN              102
#define AE_PALETTE                  103
#define AE_COLOR                    104
#define AE_AREA                     105
#define AE_PATTERN                  106
#define AE_WIN_CLOSE                107
#define AE_WIN_OUTPUT               108
#define AE_SCREEN_CLOSE             109
#define AE_PAINT                    110
#define AE_LINE                     111
#define AE_PSET                     112
#define AE_INPUT_OUT_OF_DATA        113

void _awindow_init(void);
void _awindow_shutdown(void);

void   SCREEN (short id, short width, short height, short depth, short mode, char *title);
void   WINDOW(short id, char *title, BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short flags, short scrid);
void   CLS(void);
void   LINE(BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short c, short bf);
void   SLEEP(void);
void   ON_WINDOW_CALL(void (*cb)(void));
void   LOCATE (short l, short c);
short  CSRLIN_ (void);
short  POS_ (short dummy);
ULONG  WINDOW_(short n);
void   PSET(BOOL s, short x, short y, short color);
void   PALETTE(short cid, FLOAT red, FLOAT green, FLOAT blue);
void   COLOR(short fg, short bg, short o);
void   PAINT(BOOL s, short x, short y, short pc, short bc);
void   AREA(BOOL s, short x, short y);
void   AREA_OUTLINE(BOOL enabled);
void   AREAFILL (short mode);
void   PATTERN (unsigned short lineptrn, _DARRAY_T *areaptrn);
char  *INKEY_ (void);

/*
 * ON TIMER support
 */


void _atimer_init(void);
void _atimer_shutdown(void);


#endif

