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
 * error codes
 */

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
#define AE_ON_TIMER_CALL            114
#define AE_TIMER_ON                 115
#define AE_TIMER_OFF                116
#define AE_MOUSE                    120
#define AE_BLIT                     121
#define AE_RASTPORT                 122
#define AE_FONT                     123
#define AE_AUDIO                    124

/*
 * tags
 */

typedef struct TAGITEM_ TAGITEM_t;

struct TAGITEM_
{
    ULONG ti_Tag;
    ULONG ti_Data;
};

TAGITEM_t *TAGITEMS_           (ULONG ti_Tag, ...);
ULONG     *TAGS_               (ULONG ti_Tag, ...);

/*
 * bitmaps, screens, windows, fonts, graphics
 */

typedef struct BITMAP_ BITMAP_t;

struct BITMAP_
{
    BITMAP_t       *prev, *next;
    SHORT           width, height;
    BOOL            continous;      // BOBs need all plane data allocated in one cont block
    PLANEPTR        mask;
    struct BitMap   bm;
    struct RastPort rp;
};

BITMAP_t*BITMAP_              (SHORT width, SHORT height, SHORT depth, BOOL cont);
void     BITMAP_FREE          (BITMAP_t *bm);
void     BITMAP_OUTPUT        (BITMAP_t *bm);
void     BITMAP_MASK          (BITMAP_t *bm);
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
void     FONTSTYLE            (ULONG style);
ULONG    FONTSTYLE_           (void);
SHORT    TEXTWIDTH_           (UBYTE *s);

void _awindow_init            (void);
void _awindow_shutdown        (void);

enum _aqb_output_type {_aqb_ot_none, _aqb_ot_console, _aqb_ot_window, _aqb_ot_screen, _aqb_ot_bitmap};

// consider these readonly, use _aqb_get_output() before using them to auto-open window 1 if necessary
extern struct Screen        *_g_cur_scr;
extern struct Window        *_g_cur_win;
extern struct RastPort      *_g_cur_rp ;
extern struct ViewPort      *_g_cur_vp ;
extern BITMAP_t             *_g_cur_bm ;
extern short                 _g_active_scr_id;
extern short                 _g_active_win_id;
extern short                 _g_cur_win_id;

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

typedef void (*window_close_cb_t)(short id);
void   _window_add_close_cb   (window_close_cb_t cb);

typedef BOOL (*window_msg_cb_t)(SHORT wid, struct Window *win, struct IntuiMessage *message);
void   _window_add_msg_cb     (window_msg_cb_t cb);

void   SLEEP                  (void);
void   VWAIT                  (void);
void   MOUSE_ON               (void);
void   MOUSE_OFF              (void);
void   ON_MOUSE_CALL          (void (*cb)(void));
WORD   MOUSE_                 (SHORT n);
void   MOUSE_MOTION_ON        (void);
void   MOUSE_MOTION_OFF       (void);
void   ON_MOUSE_MOTION_CALL   (void (*cb)(void));
void   LOCATE_XY              (BOOL s, SHORT x, SHORT y);
SHORT  CSRLIN_                (void);
SHORT  POS_                   (SHORT dummy);
void   LINE                   (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, SHORT c, SHORT bf);
void   PSET                   (BOOL s, SHORT x, SHORT y, SHORT color);
SHORT  POINT_                 (SHORT x, SHORT y);
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

/*
 * audio support
 */

typedef struct WAVE_ WAVE_t;

struct WAVE_
{
    WAVE_t       *prev, *next;

    ULONG         oneShotHiSamples;
    ULONG         repeatHiSamples;
    ULONG         samplesPerHiCycle;
    ULONG         samplesPerSec;
    SHORT         ctOctave;
    FLOAT         volume;

    BYTE         *data;
};

WAVE_t *_wave_alloc          (BYTE *data,
                              ULONG oneShotHiSamples, ULONG repeatHiSamples, ULONG samplesPerHiCycle,
                              ULONG samplesPerSec, SHORT ctOctave, FLOAT volume);
WAVE_t *WAVE_                (_DARRAY_T *data,
                              ULONG oneShotHiSamples, ULONG repeatHiSamples, ULONG samplesPerHiCycle,
                              ULONG samplesPerSec, SHORT ctOctave, FLOAT volume);
void    WAVE                 (SHORT channel, WAVE_t *wave);
void    WAVE_FREE            (WAVE_t *wave);
void    SOUND                (FLOAT freq, FLOAT duration, SHORT vol, SHORT channel);
void    SOUND_WAIT           (SHORT channel);
void    SOUND_STOP           (SHORT channel);
void    SOUND_START          (SHORT channel);

void _asound_init            (void);
void _asound_shutdown        (void);

#endif

