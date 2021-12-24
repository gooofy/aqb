#ifndef HAVE_AQB_H
#define HAVE_AQB_H

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <graphics/gels.h>

#include <intuition/intuition.h>

#include "../_brt/_brt.h"

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase       *GfxBase;
extern struct Library       *DiskfontBase;

/*
 * print statement support
 */

extern BPTR g_stdout;

void _aio_init       (void);
void _aio_shutdown   (void);

void _aio_puts4      (USHORT fno, LONG num);
void _aio_puts2      (USHORT fno, SHORT num);
void _aio_puts1      (USHORT fno, UBYTE num);
void _aio_putu4      (USHORT fno, ULONG num);
void _aio_putu2      (USHORT fno, USHORT num);
void _aio_putu1      (USHORT fno, UBYTE num);
void _aio_puthex     (USHORT fno, LONG num);
void _aio_putuhex    (USHORT fno, ULONG l);
void _aio_putbin     (USHORT fno, LONG num);
void _aio_putf       (USHORT fno, FLOAT f);
void _aio_putbool    (USHORT fno, BOOL b);

void _aio_puts       (USHORT fno, const UBYTE *str);
void _aio_fputs      (USHORT fno, const UBYTE *str);

void _aio_putnl      (USHORT fno);
void _aio_puttab     (USHORT fno);

struct FileHandle *_aio_getfh (USHORT fno);

// [ LINE ] INPUT support:

void _aio_gets                   (UBYTE **s, BOOL do_nl);
void _aio_line_input             (UBYTE *prompt, UBYTE **s, BOOL do_nl);
void _aio_console_input          (BOOL qm, UBYTE *prompt, BOOL do_nl);
void _aio_inputs1                (BYTE   *v);
void _aio_inputu1                (UBYTE  *v);
void _aio_inputs2                (SHORT  *v);
void _aio_inputu2                (USHORT *v);
void _aio_inputs4                (LONG   *v);
void _aio_inputu4                (ULONG  *v);
void _aio_inputf                 (FLOAT  *v);
void _aio_inputs                 (UBYTE  **v);
void _aio_set_dos_cursor_visible (BOOL visible);

void  LOCATE                     (SHORT l, SHORT c);
short CSRLIN_                    (void);
short POS_                       (SHORT dummy);

#define FILE_MODE_RANDOM      0
#define FILE_MODE_INPUT       1
#define FILE_MODE_OUTPUT      2
#define FILE_MODE_APPEND      3
#define FILE_MODE_BINARY      4

#define FILE_ACCESS_READ      0
#define FILE_ACCESS_WRITE     1
#define FILE_ACCESS_READWRITE 2

void _aio_open  (UBYTE *fname, USHORT mode, USHORT access, USHORT fno, USHORT recordlen);
void _aio_close (USHORT fno);

/*
 * bitmaps, screens, windows, fonts, graphics
 */

typedef struct BITMAP_ BITMAP_t;

struct BITMAP_
{
    BITMAP_t       *prev, *next;
    SHORT           width, height;
    BOOL            continous;      // BOBs need all plane data allocated in one cont block
    struct BitMap   bm;
    struct RastPort rp;
};

BITMAP_t*BITMAP_              (SHORT width, SHORT height, SHORT depth, BOOL cont);
void     BITMAP_FREE          (BITMAP_t *bm);
void     BITMAP_OUTPUT        (BITMAP_t *bm);
void     GET                  (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, BITMAP_t *bm);
void     PUT                  (BOOL s, SHORT x, SHORT y, BITMAP_t *bm, UBYTE minterm, BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2);

typedef struct FONT_ FONT_t;

struct FONT_
{
    FONT_t                *prev, *next;
    struct DiskFontHeader *dfh;
    struct TextFont       *tf;
};

FONT_t  *FONT_                (UBYTE *name, SHORT size, UBYTE *fontdir);
void     FONT                 (FONT_t *font);
void     FONT_FREE            (FONT_t *font);
SHORT    TEXTWIDTH_           (UBYTE *s);

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
#define AE_ON_TIMER_CALL            114
#define AE_TIMER_ON                 115
#define AE_TIMER_OFF                116
#define AE_OPEN                     117
#define AE_OUTPUT                   118
#define AE_CLOSE                    119
#define AE_MOUSE                    120
#define AE_BLIT                     121
#define AE_RASTPORT                 122
#define AE_FONT                     123

void _awindow_init            (void);
void _awindow_shutdown        (void);

enum _aqb_output_type {_aqb_ot_none, _aqb_ot_console, _aqb_ot_window, _aqb_ot_screen, _aqb_ot_bitmap};

// consider these readonly, use _aqb_get_output() before using them to auto-open window 1 if necessary
extern struct Screen        *_g_cur_scr;
extern struct Window        *_g_cur_win;
extern struct RastPort      *_g_cur_rp ;
extern struct ViewPort      *_g_cur_vp ;
extern BITMAP_t             *_g_cur_bm ;

#define MAX_NUM_WINDOWS 16
extern struct Window        *_g_winlist[MAX_NUM_WINDOWS];

enum _aqb_output_type  _aqb_get_output (BOOL needGfx);

void   SCREEN                 (SHORT id, SHORT width, SHORT height, SHORT depth, UWORD mode, UBYTE *title, BITMAP_t *bm);
void   SCREEN_CLOSE           (short id);

void   WINDOW                 (SHORT id, UBYTE *title, BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, ULONG flags, SHORT scrid);
void   WINDOW_CLOSE           (short id);
void   WINDOW_OUTPUT          (short id);
void   ON_WINDOW_CALL         (void (*cb)(void));
ULONG  WINDOW_                (SHORT n);

typedef void (*window_close_cb_t)(struct Window *win);
void   _window_add_close_cb   (window_close_cb_t cb);

void   SLEEP                  (void);
void   SLEEP_FOR              (FLOAT s);
void   VWAIT                  (void);
void   MOUSE_ON               (void);
void   MOUSE_OFF              (void);
void   ON_MOUSE_CALL          (void (*cb)(void));
WORD   MOUSE_                 (SHORT n);
void   MOUSE_MOTION_ON        (void);
void   MOUSE_MOTION_OFF       (void);
void   ON_MOUSE_MOTION_CALL   (void (*cb)(void));
void   LOCATE                 (SHORT l, SHORT c);
void   LOCATE_XY              (BOOL s, SHORT x, SHORT y);
SHORT  CSRLIN_                (void);
SHORT  POS_                   (SHORT dummy);
void   CLS                    (void);
void   LINE                   (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, SHORT c, SHORT bf);
void   PSET                   (BOOL s, SHORT x, SHORT y, SHORT color);
void   CIRCLE                 (BOOL s, SHORT x, SHORT y, SHORT r, SHORT color, SHORT start, SHORT fini, FLOAT ratio);
void   COLOR                  (SHORT fg, SHORT bg, SHORT o, SHORT drmd);
void   PAINT                  (BOOL s, SHORT x, SHORT y, SHORT pc, SHORT bc);
void   AREA                   (BOOL s, SHORT x, SHORT y);
void   AREA_OUTLINE           (BOOL enabled);
void   AREAFILL               (SHORT mode);
void   PATTERN                (USHORT lineptrn, _DARRAY_T *areaptrn);
void   PATTERN_RESTORE        (void);

char  *INKEY_                 (void);

/*
 * palettes
 */

typedef struct
{
    UBYTE           r, g, b;
} COLOR_t;

typedef struct
{
    SHORT           numEntries;
    COLOR_t         colors[256];
} PALETTE_t;

void     PALETTE              (SHORT cid, FLOAT red, FLOAT green, FLOAT blue);
void     PALETTE_LOAD         (PALETTE_t *p);
void     _palette_load        (SHORT scid, PALETTE_t *p);

/*
 * ON TIMER support
 */

extern ULONG _g_signalmask_atimer;

void _atimer_init(void);
void _atimer_shutdown(void);
void _atimer_process_signals(ULONG signals);

void ON_TIMER_CALL     (SHORT id, FLOAT d, void (*cb)(void));
void TIMER_ON          (SHORT id);
void TIMER_OFF         (SHORT id);

#endif

