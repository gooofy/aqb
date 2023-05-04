//#define ENABLE_DPRINTF

#include "_aqb.h"
#include "../_brt/_brt.h"

#include <stdarg.h>
#include <string.h>

#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <inline/intuition.h>

#include <clib/graphics_protos.h>
#include <inline/graphics.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#include <clib/mathtrans_protos.h>
#include <inline/mathtrans.h>

#include <proto/console.h>
#include <clib/console_protos.h>
#include <pragmas/console_pragmas.h>

#include <libraries/diskfont.h>
#include <inline/diskfont.h>

// #define ENABLE_DEBUG

#define MAXINPUTBUF 1024

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

struct Device           *ConsoleDevice;
static FLOAT             g_fp15; // FFP representation of decimal 15, used in PALETTE
static FLOAT             g_fp50; // FFP representation of decimal 50, used in _awindow_sleep_for
static struct IOStdReq   g_ioreq; // console.device is used to convert RAWKEY codes
static BOOL              g_console_device_opened=FALSE;
static struct InputEvent g_ievent;
static BPTR              g_stdout, g_stdin;

static struct NewWindow g_nw =
{
    0, 0, 0, 0,                                                    // LeftEdge, TopEdge, Width, Height
    0, 1,                                                          // DetailPen, BlockPen
    0,                                                             // IDCMPFlags
    0,                                                             // Flags
    NULL,                                                          // FirstGadget
    NULL,                                                          // CheckMark
    NULL,                                                          // Title
    NULL,                                                          // Screen
    NULL,                                                          // BitMap
    -1, -1, -1, -1,                                                // MinWidth, MinHeight, MaxWidth, MaxHeight
    0                                                              // Type
};

/*
 * keep track of window and screen ids
 */

typedef struct win_close_cb_node_s *win_close_cb_node_t;
struct win_close_cb_node_s
{
    win_close_cb_node_t next;
    window_close_cb_t   cb;
    void               *ud;
};

typedef struct win_msg_cb_node_s *win_msg_cb_node_t;
struct win_msg_cb_node_s
{
    win_msg_cb_node_t next;
    window_msg_cb_t   cb;
};

typedef struct
{
    struct Window       *win;

    window_close_cb_t    aqb_close_cb;
    void                *aqb_close_ud;
    window_newsize_cb_t  aqb_newsize_cb;
    void                *aqb_newsize_ud;
    window_refresh_cb_t  aqb_refresh_cb;
    void                *aqb_refresh_ud;
    win_close_cb_node_t  close_cbs;
    win_msg_cb_node_t    msg_cbs;

} AQBWindow_t;

static AQBWindow_t _g_winlist[MAX_NUM_WINDOWS];

#define MAX_NUM_SCREENS 4
static struct Screen * g_scrlist[MAX_NUM_SCREENS] = { NULL, NULL, NULL, NULL };

static struct NewScreen g_nscr =
{
    0, 0,                   // LeftEdge, TopEdge
    640, 256,               // Width, Height
    2,                      // Depth
    0, 1,                   // DetailPen, BlockPen
    HIRES,                  // ViewModes
    CUSTOMSCREEN,           // Type
    NULL,                   // Font
    (UBYTE *) "AQB SCREEN", // DefaultTitle
    NULL,                   // UNUSED
    NULL                    // CustomBitMap
};

static ULONG                 _g_signalmask_awindow = 0;

static BITMAP_t             *g_bm_first            = NULL;
static BITMAP_t             *g_bm_last             = NULL;

static FONT_t               *g_font_first          = NULL;
static FONT_t               *g_font_last           = NULL;

static mouse_cb_t            g_mouse_cb            = NULL;
static void                 *g_mouse_ud            = NULL;
static mouse_cb_t            g_mouse_motion_cb     = NULL;
static void                 *g_mouse_motion_ud     = NULL;

short                 _g_active_scr_id = 0;
short                 _g_active_win_id = 1;
short                 _g_cur_win_id    = 1;

struct Screen        *_g_cur_scr    = NULL;
struct Window        *_g_cur_win    = NULL;
struct RastPort      *_g_cur_rp     = NULL;
struct ViewPort      *_g_cur_vp     = NULL;
BITMAP_t             *_g_cur_bm     = NULL;

static enum _aqb_output_type g_cur_ot        = _aqb_ot_none;

typedef struct myTimeVal
{
    ULONG LeftSeconds;
    ULONG LeftMicros;
} MYTIMEVAL;

static BOOL             g_mouse_bev     = FALSE;
static BOOL             g_mouse_dc      = FALSE; // dc: double click
static BOOL             g_mouse_down    = FALSE;
static MYTIMEVAL        g_mouse_tv      = { 0, 0 };
static WORD             g_mouse_down_x  = 0;
static WORD             g_mouse_down_y  = 0;
static WORD             g_mouse_up_x    = 0;
static WORD             g_mouse_up_y    = 0;

void SCREEN (SHORT id, SHORT width, SHORT height, SHORT depth, UWORD mode, UBYTE *title, BITMAP_t *bm)
{
    CHKBRK;

    // error checking
    if ( (id < 1) || (id > MAX_NUM_SCREENS) || (g_scrlist[id-1] != NULL) || (width <=0) || (height <= 0) || (depth <= 0) || (depth>6) )
    {
        ERROR(AE_SCREEN_OPEN);
        return;
    }

    //DPRINTF ("SCREEN: bm=0x%08lx\n", (ULONG)bm);

    //_debug_puts((STRPTR)"SCREEN title: "); _debug_puts(title); _debug_putnl();

    g_nscr.Width        = width;
    g_nscr.Height       = height;
    g_nscr.Depth        = depth;
    g_nscr.DefaultTitle = title ? (UBYTE *)_astr_dup(title) : (UBYTE*) "";
    g_nscr.ViewModes    = mode;
    g_nscr.CustomBitMap = bm ? &bm->bm : NULL;
    g_nscr.Type         = bm ? CUSTOMSCREEN|CUSTOMBITMAP|SCREENQUIET : CUSTOMSCREEN;

    // _debug_puts((STRPTR)"g_nscr.ViewModes:"); _debug_puts2(g_nscr.ViewModes); _debug_puts((STRPTR)"");

    struct Screen *scr = (struct Screen *)OpenScreen(&g_nscr);

    if (!scr)
    {
        ERROR(AE_SCREEN_OPEN);
        return;
    }

    g_scrlist[id-1]  = scr;
    _g_cur_scr       = scr;
    _g_active_scr_id = id;
	_g_cur_vp        = &scr->ViewPort;
    g_cur_ot         = _aqb_ot_screen;
    _g_cur_rp        = &scr->RastPort;
}

/*
 * SCREEN CLOSE id
 */
void SCREEN_CLOSE(short id)
{
    // error checking
    if ( (id < 1) || (id > MAX_NUM_SCREENS) || (g_scrlist[id-1] == NULL) )
    {
        ERROR(AE_SCREEN_CLOSE);
        return;
    }
    CloseScreen(g_scrlist[id-1]);
    g_scrlist[id-1]=NULL;
}

/*
 * WINDOW id [, [Title] [, [(x1,y1)-(x2,y2)] [, [Flags] [, Screen] ] ]
 */
void WINDOW(SHORT id, UBYTE *title, BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, ULONG flags, SHORT scrid)
{
    CHKBRK;

    USHORT w, h;

    // error checking
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (_g_winlist[id-1].win != NULL) || (x1 > x2) || (y1 > y2) )
    {
        ERROR(AE_WIN_OPEN);
        return;
    }

    if (x1>=0)
    {
        w  = x2 - x1;
        h  = y2 - y1;
    }
    else
    {

        x1 = 0;
        y1 = 0;

        if (!_g_cur_scr)
        {
            struct Screen sc;
            // get workbench screen size, limit to 640x200 for now (FIXME: limit to visible screen size?)
            if (!GetScreenData ((APTR) &sc, sizeof(struct Screen), WBENCHSCREEN, NULL))
            {
                ERROR(AE_WIN_OPEN);
                return;
            }

            w = sc.Width  > 640 ? 640 : sc.Width;
            h = sc.Height > 200 ? 200 : sc.Height;
        }
        else
        {
            w = _g_cur_scr->Width;
            h = _g_cur_scr->Height;
        }
    }

    g_nw.LeftEdge   = x1;
    g_nw.TopEdge    = y1;
    g_nw.Width      = w;
    g_nw.Height     = h;
    g_nw.Title      = title ? (UBYTE *) _astr_dup(title) : (UBYTE*) "";

    g_nw.Flags      = flags;
    g_nw.IDCMPFlags = CLOSEWINDOW | RAWKEY | ACTIVEWINDOW | IDCMP_REFRESHWINDOW; // INTUITICKS | VANILLAKEY | MENUPICK | GADGETUP | ACTIVEWINDOW;

    if (_g_cur_scr)
    {
        g_nw.Screen   = _g_cur_scr;
        g_nw.Type     = CUSTOMSCREEN;
    }
    else
    {
        g_nw.Screen   = NULL;
        g_nw.Type     = WBENCHSCREEN;
    }

    //struct Window *win = (struct Window *)OpenWindow(&g_nw);

    struct Window *win = OpenWindowTags (&g_nw, WA_NewLookMenus, TRUE, TAG_DONE);

    if (!win)
    {
        ERROR(AE_WIN_OPEN);
        return;
    }

    _g_winlist[id-1].win = win;

    _g_signalmask_awindow |= (1L << win->UserPort->mp_SigBit);

    _g_cur_win    = win;
    _g_cur_rp     = win->RPort;
    _g_cur_win_id = id;
    g_cur_ot      = _aqb_ot_window;

    LOCATE (1,1);
    COLOR (1, 0, 1, JAM2);
}

/*
 * WINDOW CLOSE id
 */
void WINDOW_CLOSE(short id)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("WINDOW_CLOSE id=%d\n", id);
#endif

    // error checking
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (_g_winlist[id-1].win == NULL) )
    {
        ERROR(AE_WIN_CLOSE);
        return;
    }

    // call close callbacks first
    for (win_close_cb_node_t n=_g_winlist[id-1].close_cbs; n; n=n->next)
    {
#ifdef ENABLE_DEBUG
        DPRINTF ("WINDOW_CLOSE calling close cb 0x%08lx)\n", n->cb);
#endif
        n->cb(id, n->ud);
    }

    if (_g_winlist[id-1].win->RPort->TmpRas)
    {
        FreeVec ((PLANEPTR) _g_winlist[id-1].win->RPort->TmpRas->RasPtr);
        FreeVec (_g_winlist[id-1].win->RPort->TmpRas);
    }
#ifdef ENABLE_DEBUG
    DPRINTF ("WINDOW_CLOSE id=%d CloseWindow(0x%08lx)\n", id, _g_winlist[id-1].win);
#endif
    CloseWindow(_g_winlist[id-1].win);
    _g_winlist[id-1].win=NULL;
}

void _window_add_close_cb (SHORT win_id, window_close_cb_t cb, void *ud)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("_window_add_close_cb win_id=%d, cb=0x%08lx\n", win_id, cb);
#endif
    win_close_cb_node_t node = ALLOCATE_(sizeof (*node), 0);
    if (!node)
    {
        ERROR (AE_WIN_CLOSE);
        return;
    }
    node->next = _g_winlist[win_id-1].close_cbs;
    node->cb   = cb;
    node->ud   = ud;
    _g_winlist[win_id-1].close_cbs = node;
}

void _window_add_msg_cb (SHORT win_id, window_msg_cb_t cb)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("_window_add_msg_cb win_id=%d, cb=0x%08lx\n", win_id, cb);
#endif
    win_msg_cb_node_t node = ALLOCATE_(sizeof (*node), 0);
    if (!node)
    {
        ERROR (AE_WIN_CLOSE);
        return;
    }
    node->next = _g_winlist[win_id-1].msg_cbs;
    node->cb   = cb;
    _g_winlist[win_id-1].msg_cbs = node;
}

/*
 * WINDOW OUTPUT id
 */
void WINDOW_OUTPUT(short id)
{
    // switch (back) to console output?
    if ( (id == 1) && !_g_winlist[0].win && (_startup_mode == STARTUP_CLI) )
    {
        _g_cur_win_id = 1;
        _g_cur_win    = NULL;
        _g_cur_rp     = NULL;
        g_cur_ot      = _aqb_ot_console;
        return;
    }

    // error checking
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (_g_winlist[id-1].win == NULL) )
    {
        ERROR(AE_WIN_OUTPUT);
        return;
    }

    struct Window *win = _g_winlist[id-1].win;

    _g_cur_win_id = id;
    _g_cur_win    = win;
    _g_cur_rp     = win->RPort;
    g_cur_ot      = _aqb_ot_window;
}

void _awindow_shutdown(void)
{
#ifdef ENABLE_DEBUG
    DPRINTF("_awindow_shutdown ...\n");
#endif
    for (int i = 0; i<MAX_NUM_WINDOWS; i++)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_awindow_shutdown _g_winlist[%d].win=0x%08lx\n", i, _g_winlist[i].win);
#endif
        if (_g_winlist[i].win)
            WINDOW_CLOSE(i+1);
    }
    for (int i = 0; i<MAX_NUM_SCREENS; i++)
    {
        if (g_scrlist[i])
        {
#ifdef ENABLE_DEBUG
            DPRINTF("_awindow_shutdown ... CloseScreen\n");
            //Delay (100);
#endif
            CloseScreen(g_scrlist[i]);
        }
    }

    BITMAP_t *bm = g_bm_first;
    while (bm)
    {
        BITMAP_t *next = bm->next;
        BITMAP_FREE(bm);
        bm = next;
    }

    FONT_t *font = g_font_first;
    while (font)
    {
        FONT_t *next = font->next;
        FONT_FREE(font);
        font = next;
    }

    if (g_console_device_opened)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_awindow_shutdown ... CloseDevice\n");
        //Delay (100);
#endif
        CloseDevice((struct IORequest *)&g_ioreq);
    }
#ifdef ENABLE_DEBUG
    DPRINTF("_awindow_shutdown ... finished\n");
    //Delay (100);
#endif
}

enum _aqb_output_type  _aqb_get_output (BOOL needGfx)
{
    CHKBRK;

    // auto-open graphical window 1 ?
    if (needGfx)
    {
        if ((g_cur_ot == _aqb_ot_console) || (g_cur_ot == _aqb_ot_none) )
        {
            WINDOW (/*id=*/1, /*title=*/(STRPTR)"AQB Output",
                    /*s1=*/FALSE, /*x1=*/-1, /*y1=*/-1,
                    /*s2=*/FALSE, /*x2=*/-1, /*y2=*/-1,
                    /*flags=*/WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_SMART_REFRESH | WFLG_GIMMEZEROZERO | WFLG_ACTIVATE, /*scrid=*/0);
        }

        if (!_g_cur_rp)
        {
            ERROR (AE_RASTPORT);
            return g_cur_ot;
        }
    }
    else
    {
        if (g_cur_ot == _aqb_ot_none)
        {
            WINDOW (/*id=*/1, /*title=*/(STRPTR)"AQB Output",
                    /*s1=*/FALSE, /*x1=*/-1, /*y1=*/-1,
                    /*s2=*/FALSE, /*x2=*/-1, /*y2=*/-1,
                    /*flags=*/WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_SMART_REFRESH | WFLG_GIMMEZEROZERO | WFLG_ACTIVATE, /*scrid=*/0);
        }
    }

    return g_cur_ot;
}

struct Window *_aqb_get_win (SHORT wid)
{
    struct Window *win = _g_winlist[wid-1].win;
    return win;
}

SHORT _aqb_get_win_id (struct Window *win)
{
    for (int i =0; i<MAX_NUM_WINDOWS; i++)
    {
        if (_g_winlist[i].win==win)
            return i+1;
    }
    return 0;
}

static BOOL _awindow_cls (void)
{
    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {
        char form_feed = 0x0c;
        Write(g_stdout, (CONST APTR) &form_feed, 1);
        return TRUE;
    }

    Move (_g_cur_rp, 0, 0);
    ClearScreen(_g_cur_rp);
    LOCATE(1, 1);
    return TRUE;
}

/*
 * LINE [ [ STEP ] ( x1 , y1 ) ] - [ STEP ] ( x2 , y2 ) [, [ Color ]  [, flag ] ]
 */
void LINE(BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short c, short bf)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    BYTE fgPen=_g_cur_rp->FgPen;
    if (x1<0)
        x1 = _g_cur_rp->cp_x;
    if (y1<0)
        y1 = _g_cur_rp->cp_y;

    if (s1)
    {
        x1 += _g_cur_rp->cp_x;
        y1 += _g_cur_rp->cp_y;
    }
    if (s2)
    {
        x2 += _g_cur_rp->cp_x;
        y2 += _g_cur_rp->cp_y;
    }
    if (c >= 0)
        SetAPen(_g_cur_rp, c);
    if (bf & 1)
    {
        if (bf & 2)
        {
            RectFill (_g_cur_rp, x1, y1, x2, y2);
        }
        else
        {
            Move (_g_cur_rp, x1, y1);
            Draw (_g_cur_rp, x2, y1);
            Draw (_g_cur_rp, x2, y2);
            Draw (_g_cur_rp, x1, y2);
            Draw (_g_cur_rp, x1, y1);
        }
    }
    else
    {
        Move (_g_cur_rp, x1, y1);
        Draw (_g_cur_rp, x2, y2);
    }

    if ( c >=0 )
        SetAPen(_g_cur_rp, fgPen);
}

void PSET(BOOL s, short x, short y, short color)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    BYTE fgPen=_g_cur_rp->FgPen;

    if (s)
    {
        x += _g_cur_rp->cp_x;
        y += _g_cur_rp->cp_y;
    }
    // if (flags & AW_PSET_RESET)
    //    SetAPen(_g_cur_rp, _g_cur_rp->BgPen);
    if (color >= 0)
        SetAPen(_g_cur_rp, color);

    Move (_g_cur_rp, x, y);
    WritePixel(_g_cur_rp, x, y);

    //if ( (flags & AW_PSET_RESET) || color >=0 )
    //    SetAPen(_g_cur_rp, fgPen);
    if ( color >=0 )
        SetAPen(_g_cur_rp, fgPen);
}

SHORT POINT_ (SHORT x, SHORT y)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    return ReadPixel (_g_cur_rp, x, y);
}

void CIRCLE (BOOL s, SHORT x, SHORT y, SHORT ry, SHORT color, SHORT start, SHORT fini, FLOAT ratio)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("CIRCLE: x=%d, y=%d, ry=%d, color=%d, start=%d, end=%d\n", x, y, ry, color, start, fini);
#endif

    _aqb_get_output (/*needGfx=*/TRUE);
    BYTE fgPen=_g_cur_rp->FgPen;

    if (s)
    {
        x += _g_cur_rp->cp_x;
        y += _g_cur_rp->cp_y;
    }

    if (color >= 0)
        SetAPen(_g_cur_rp, color);

    SHORT rx = SPFix(SPDiv (ratio, SPFlt(ry)));
    if ((start == 0) && (fini == 359))
    {
        DrawEllipse (_g_cur_rp, x, y, rx, ry);
    }
	else
    {
        // FLOAT xold, yold, xnew, ynew;
        // FLOAT sin_rxbyry, sin_rybyrx, costheta, theta;
        // int num;

        FLOAT min_theta = SPDiv(SPFlt(100), SPFlt(1));

        FLOAT theta = MAX(SPFlt(1) / MAX(rx, ry), min_theta);

        // DPRINTF ("theta*1000 = %d, min_theta*1000 = %d\n", SPFix(SPMul(theta, SPFlt(1000))), SPFix(SPMul(min_theta, SPFlt(1000))));

        FLOAT M_PI = SPFlt(314159) / SPFlt(100000);
        // DPRINTF ("M_PI = %d\n", SPFix(M_PI));

        int num = SPFix(M_PI * SPFlt(SPAbs (fini-start)) / SPFlt(180) / theta);

        // DPRINTF ("num=%d\n", num);

        FLOAT rrx = SPFlt(rx);
        FLOAT rry = SPFlt(ry);

        FLOAT sin_rxbyry = rrx/rry * SPSin(theta);
        FLOAT sin_rybyrx = rry/rrx * SPSin(theta);
        FLOAT costheta = SPCos(theta);

        FLOAT xold = rrx * SPCos(M_PI * start / SPFlt(180));
        FLOAT yold = rry * SPSin(M_PI * start / SPFlt(180));

        WORD cp_x = _g_cur_rp->cp_x;
        WORD cp_y = _g_cur_rp->cp_y;

        Move (_g_cur_rp, x+SPFix(xold), y+SPFix(yold));

        for(; num; num--)
        {
            // DPRINTF ("num: %d\n", num);
            FLOAT xnew = xold * costheta - yold * sin_rxbyry;
            FLOAT ynew = xold * sin_rybyrx + yold * costheta;
            // DPRINTF ("xnew=%d, ynew=%d\n", SPFix(xnew), SPFix(ynew));
            //mappixel((int)rint(xnew+xc), (int)rint(ynew+yc));
            xold = xnew, yold = ynew;
            Draw (_g_cur_rp, x+SPFix(xold), y+SPFix(yold));
        }

        Move (_g_cur_rp, cp_x, cp_y);
	}

    if ( color >=0 )
        SetAPen(_g_cur_rp, fgPen);
}

/* BASIC: SLEEP
   event handling */

#define MAXKEYBUF 4

static char keybuf[MAXKEYBUF];
static USHORT keybuf_start = 0;
static USHORT keybuf_end   = 0;
static USHORT keybuf_len   = 0;

/* Convert RAWKEYs into VANILLAKEYs, also shows special keys like HELP, Cursor Keys,
** FKeys, etc.  It returns:
**   -2 if not a RAWKEY event.
**   -1 if not enough room in the buffer, try again with a bigger buffer.
**   otherwise, returns the number of characters placed in the buffer.
*/
LONG deadKeyConvert(struct IntuiMessage *msg, UBYTE *kbuffer, LONG kbsize)
{
    if (msg->Class != IDCMP_RAWKEY) return(-2);
    g_ievent.ie_Class = IECLASS_RAWKEY;
    g_ievent.ie_Code = msg->Code;
    g_ievent.ie_Qualifier = msg->Qualifier;
    g_ievent.ie_position.ie_addr = *((APTR*)msg->IAddress);

    LONG n = RawKeyConvert(&g_ievent, kbuffer, kbsize, /*kmap=*/NULL);

#ifdef ENABLE_DEBUG
    //_debug_puts((STRPTR)"deadKeyConv: n="); _debug_putu4(n);
    //_debug_puts((STRPTR)", Code="); _debug_putu4(msg->Code);
    //_debug_puts((STRPTR)", Qual="); _debug_putu4(msg->Qualifier);

    //for (int i=0; i<n; i++)
    //{
    //    _debug_puts((STRPTR)", kb["); _debug_putu2(i); _debug_puts((STRPTR)"]=");
    //    _debug_putu1(kbuffer[i]);
    //}

    //_debug_putnl();
#endif

    return n;
}

static WORD _winMouseX(struct Window *win)
{
    BOOL gzz = win->Flags & WFLG_GIMMEZEROZERO;
    //DPRINTF ("_winMouseX: win->Flags=0x%08lx, WFLG_GIMMEZEROZERO=0x%08lx -> gzz=%d\n",
    //         win->Flags, WFLG_GIMMEZEROZERO, gzz);
    //DPRINTF ("_winMouseX: win->GZZMouseX=%d, win->MouseX=%d\n",
    //         win->GZZMouseX, win->MouseX);
    return gzz ? win->GZZMouseX : win->MouseX;
}

static WORD _winMouseY(struct Window *win)
{
    return win->Flags & WFLG_GIMMEZEROZERO ? win->GZZMouseY : win->MouseY;
}

static void _handleSignals(BOOL doWait)
{
    CHKBRK;

    struct IntuiMessage *message = NULL;

    ULONG signals = 0;

	if (doWait)
		signals = Wait (_g_signalmask_awindow | _g_signalmask_atimer);
	else
		signals = SetSignal (0, _g_signalmask_awindow | _g_signalmask_atimer);

#ifdef ENABLE_DEBUG
	DPRINTF("_handleSignals: signals=0x%08lx\n", signals);
    //Delay(100);
#endif

    if (signals & _g_signalmask_atimer)
        _atimer_process_signals(signals);

    for (int wid=0; wid<MAX_NUM_WINDOWS; wid++)
    {
        struct Window *win = _g_winlist[wid].win;
        if (!win)
            continue;

#ifdef ENABLE_DEBUG
        DPRINTF("_handleSignals: checking wid=%d, win=0x%08lx\n", wid, win);
        //Delay(50);
#endif
        if (!(signals & (1L << win->UserPort->mp_SigBit)) )
            continue;

        while ( (message = (struct IntuiMessage *)GetMsg(win->UserPort) ) )
        {
            ULONG class = message->Class;

#ifdef ENABLE_DEBUG
            DPRINTF("_handleSignals: got a message, class=0x%08lx, wid=%d\n", class, wid);
            //Delay(100);
#endif

            BOOL handled = FALSE;

            for (win_msg_cb_node_t node = _g_winlist[wid].msg_cbs; node; node=node->next)
            {
#ifdef ENABLE_DEBUG
                DPRINTF("_handleSignals: calling msg_cb 0x%08lx\n", node->cb);
#endif
                if (node->cb (wid, win, message, _g_winlist[wid].aqb_refresh_cb, _g_winlist[wid].aqb_refresh_ud))
                {
                    handled = TRUE;
                    break;
                }
            }

            if (!handled)
            {
                switch(class)
                {
                    case CLOSEWINDOW:
#ifdef ENABLE_DEBUG
                        DPRINTF ("_handleSignals: CLOSEWINDOW wid=%d, aqb_close_cb=0x%08lx\n", wid, _g_winlist[wid].aqb_close_cb);
                        for (int j=0; j<MAX_NUM_WINDOWS; j++)
                            DPRINTF ("                            j=%d, aqb_close_cb=0x%08lx\n", j, _g_winlist[j].aqb_close_cb);
#endif

                        if (_g_winlist[wid].aqb_close_cb)
                        {
                            _g_winlist[wid].aqb_close_cb(wid+1, _g_winlist[wid].aqb_close_ud);
                        }
                        break;

                    case IDCMP_NEWSIZE:
                        if (_g_winlist[wid].aqb_newsize_cb)
                        {
                            WORD w = _g_winlist[wid].win->Flags & WFLG_GIMMEZEROZERO ? _g_winlist[wid].win->GZZWidth  : _g_winlist[wid].win->Width ;
                            WORD h = _g_winlist[wid].win->Flags & WFLG_GIMMEZEROZERO ? _g_winlist[wid].win->GZZHeight : _g_winlist[wid].win->Height;
                            _g_winlist[wid].aqb_newsize_cb(wid+1, w, h, _g_winlist[wid].aqb_newsize_ud);
                        }
                        break;

                    case MOUSEBUTTONS:
                    {
                        WORD mx = _winMouseX(win);
                        WORD my = _winMouseY(win);
                        switch (message->Code)
                        {
                            case SELECTDOWN:
                                g_mouse_down = TRUE;
                                g_mouse_bev  = TRUE;
                                g_mouse_dc = DoubleClick(g_mouse_tv.LeftSeconds, g_mouse_tv.LeftMicros, message->Seconds, message->Micros);
                                if (!g_mouse_dc)
                                {
                                    g_mouse_tv.LeftSeconds = message->Seconds;
                                    g_mouse_tv.LeftMicros  = message->Micros;
                                }
                                g_mouse_down_x = mx;
                                g_mouse_down_y = my;
                                break;
                            case SELECTUP:
                                g_mouse_bev  = TRUE;
                                g_mouse_down = FALSE;
                                g_mouse_up_x = mx;
                                g_mouse_up_y = my;
                                break;
                        }

                        if (g_mouse_cb)
                        {
                            g_mouse_cb(wid, g_mouse_down, mx, my, g_mouse_ud);
                        }
                        break;
                    }
                    case MOUSEMOVE:
                        if (g_mouse_motion_cb)
                        {
                            WORD mx = _winMouseX(win);
                            WORD my = _winMouseY(win);
                            g_mouse_motion_cb(wid, g_mouse_down, mx, my, g_mouse_ud);
                        }
                        break;

                    case ACTIVEWINDOW:
                        _g_active_win_id = wid+1;
                        break;

                    case RAWKEY:
                    {
                        UBYTE buf[12];
                        LONG numchars = deadKeyConvert(message, buf, 11);
                        switch (numchars)
                        {
                            case 1:
                                if (keybuf_len < MAXKEYBUF)
                                {
                                    keybuf[keybuf_end] = buf[0];
                                    keybuf_end = (keybuf_end + 1) % MAXKEYBUF;
                                    keybuf_len++;
                                }
                                break;

                            case 2:
                            case 3:
                            {
                                char code=0;
                                switch (buf[1])
                                {
                                    // cursor keys
                                    case 65:
                                    case 66:
                                    case 67:
                                    case 68:
                                        code = buf[1]-37;
                                        break;
                                    // function keys
                                    case 48:
                                    case 49:
                                    case 50:
                                    case 51:
                                    case 52:
                                    case 53:
                                    case 54:
                                    case 55:
                                    case 56:
                                    case 57:
                                        code = buf[1]+81;
                                        break;
                                    default:
                                        break;
                                    }
                                    if (code)
                                    {
                                        if (keybuf_len < MAXKEYBUF)
                                        {
                                            keybuf[keybuf_end] = code;
                                            keybuf_end = (keybuf_end + 1) % MAXKEYBUF;
                                            keybuf_len++;
                                        }
                                    }
                                    break;
                            }
                            default:
                                break;
                        }
                        break;
                    }
                    case IDCMP_REFRESHWINDOW:
                        BeginRefresh (win);
                        if (_g_winlist[wid].aqb_refresh_cb)
                            _g_winlist[wid].aqb_refresh_cb(wid+1, _g_winlist[wid].aqb_refresh_ud);
                        EndRefresh (win, TRUE);
                        break;
                }
            }

#ifdef ENABLE_DEBUG
            DPRINTF("_handleSignals: ReplyMsg...\n");
            //Delay(100);
#endif
            ReplyMsg ( (struct Message *) message);
#ifdef ENABLE_DEBUG
            DPRINTF("_handleSignals: ReplyMsg... done.\n");
            //Delay(100);
#endif
        }
    }
#ifdef ENABLE_DEBUG
    DPRINTF("_handleSignals: done.\n");
    //Delay(100);
#endif
}

void SLEEP(void)
{
	_handleSignals(/*doWait=*/TRUE);
}

static void _awindow_sleep_for (FLOAT s)
{
    LONG ticks = SPFix(SPMul(s, g_fp50));

    if (ticks <= 0)
        return;

    while (ticks > 25)
    {
		_handleSignals(/*doWait=*/FALSE);
        Delay (25);
        ticks -= 25;
    }

    Delay (ticks);
	_handleSignals(/*doWait=*/FALSE);
}

void VWAIT (void)
{
	WaitTOF();
	_handleSignals(/*doWait=*/FALSE);
}

void ON_WINDOW_CLOSE_CALL(short id, window_close_cb_t cb, void *ud)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("ON_WINDOW_CLOSE_CALL: id=%d, cb=0x%08lx\n", id, cb);
    //Delay (100);
#endif
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) )
    {
        ERROR(AE_WIN_CALL);
        return;
    }
    AQBWindow_t *aqbw = &_g_winlist[id-1];
    aqbw->aqb_close_cb = cb;
#ifdef ENABLE_DEBUG
    DPRINTF ("ON_WINDOW_CLOSE_CALL: aqbw=0x%08lx\n", aqbw);
    //for (int j=0; j<MAX_NUM_WINDOWS; j++)
    //    DPRINTF ("                            j=%d, aqb_close_cb=0x%08lx\n", j, _g_winlist[j].aqb_close_cb);
    //Delay (100);
#endif
}

void ON_WINDOW_NEWSIZE_CALL (SHORT id, window_newsize_cb_t cb, void *ud)
{
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) )
    {
        ERROR(AE_WIN_CALL);
        return;
    }
    AQBWindow_t *aqbw = &_g_winlist[id-1];
    aqbw->aqb_newsize_cb = cb;
    aqbw->aqb_newsize_ud = ud;
    ModifyIDCMP (aqbw->win, aqbw->win->IDCMPFlags | IDCMP_NEWSIZE);
}

void ON_WINDOW_REFRESH_CALL (SHORT id, window_refresh_cb_t cb, void *ud)
{
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) )
    {
        ERROR(AE_WIN_CALL);
        return;
    }
    AQBWindow_t *aqbw = &_g_winlist[id-1];
    aqbw->aqb_refresh_cb = cb;
    aqbw->aqb_refresh_ud = ud;
}

ULONG WINDOW_(short n)
{
    switch(n)
    {
        case 0:                                 //  0: current active window
            return _g_active_win_id;
        case 1:                                 //  1: current output window id
            return _g_cur_win_id;
        case 2:                                 //  2: current output window width
            if (!_g_cur_win)
                return 0;
            return _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? _g_cur_win->GZZWidth : _g_cur_win->Width;
        case 3:                                 //  3: current output window height
            if (!_g_cur_win)
                return 0;
            return _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? _g_cur_win->GZZHeight : _g_cur_win->Height;
        case 4:                                 //  4: current output cursor X
            if (!_g_cur_rp)
                return 0;
            return _g_cur_rp->cp_x;
        case 5:                                 //  5: current output cursor Y
            if (!_g_cur_rp)
                return 0;
            return _g_cur_rp->cp_y;
        case 6:                                 //  6: highest color index
            if (!_g_cur_rp)
                return 0;
            return (1<<_g_cur_rp->BitMap->Depth)-1;
        case 7:                                 //  7: pointer to current intuition output window
            return (ULONG) _g_cur_win;
        case 8:                                 //  8: pointer to current rastport
            return (ULONG) _g_cur_rp;
        case 9:                                 //  9: output file handle (ACE)
            return (ULONG) g_stdout;
        case 10:                                // 10: foreground pen (ACE)
            if (!_g_cur_rp)
                return 0;
            return _g_cur_rp->FgPen;
        case 11:                                // 11: background pen (ACE)
            if (!_g_cur_rp)
                return 0;
            return _g_cur_rp->BgPen;
        case 12:                                // 12: text width (ACE)
            if (!_g_cur_rp)
                return 0;
            return _g_cur_rp->TxWidth;
        case 13:                                // 13: text height (ACE)
            if (!_g_cur_rp)
                return 0;
            return _g_cur_rp->TxHeight;
        case 14:                                // 14: input file handle (AQB)
            return (ULONG) g_stdin;

    }
    return 0;
}

void MOUSE_ON (void)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    if ( !_g_cur_win )
    {
        ERROR(AE_MOUSE);
        return;
    }

    ModifyIDCMP (_g_cur_win, _g_cur_win->IDCMPFlags | MOUSEBUTTONS);
}

void MOUSE_OFF (void)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    if ( !_g_cur_win )
    {
        ERROR(AE_MOUSE);
        return;
    }

    ModifyIDCMP (_g_cur_win, _g_cur_win->IDCMPFlags & ~MOUSEBUTTONS);
}

void ON_MOUSE_CALL (mouse_cb_t cb, void *ud)
{
    g_mouse_cb = cb;
    g_mouse_ud = ud;
}

WORD MOUSE_ (SHORT n)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    if ( !_g_cur_win )
    {
        ERROR(AE_MOUSE);
        return 0;
    }

    WORD res = 0;

    switch (n)
    {
        case 0:
			if (g_mouse_bev)
			{
				if (g_mouse_down)
					res = g_mouse_dc ? -2 : -1;
				else
					res = g_mouse_dc ? 2 : 1;
				g_mouse_bev = FALSE;
			}
            break;

        case 1:
            return _winMouseX(_g_cur_win);
        case 2:
            return _winMouseY(_g_cur_win);

        case 3:
            return g_mouse_down_x;
        case 4:
            return g_mouse_down_y;

        case 5:
            return g_mouse_up_x;
        case 6:
            return g_mouse_up_y;

        default:
            ERROR(AE_MOUSE);
    }

    return res;
}

void MOUSE_MOTION_ON (void)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    if ( !_g_cur_win )
    {
        ERROR(AE_MOUSE);
        return;
    }

    _g_cur_win->Flags |= REPORTMOUSE;
    ModifyIDCMP (_g_cur_win, _g_cur_win->IDCMPFlags | MOUSEMOVE);
}

void MOUSE_MOTION_OFF (void)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    if ( !_g_cur_win )
    {
        ERROR(AE_MOUSE);
        return;
    }

    _g_cur_win->Flags &= ~REPORTMOUSE;
    ModifyIDCMP (_g_cur_win, _g_cur_win->IDCMPFlags & ~MOUSEMOVE);
}

void ON_MOUSE_MOTION_CALL (mouse_cb_t cb, void *ud)
{
    g_mouse_motion_cb = cb;
    g_mouse_motion_ud = ud;
    //_debug_puts((STRPTR)"ON_MOUSE_MOTION_CALL\n");
}

/*
 * print statement support
 */

static void do_scroll(void)
{
    WORD win_x      = _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? 0 : _g_cur_win->BorderLeft;
    WORD win_y      = _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? 0 : _g_cur_win->BorderTop;
    WORD win_width  = _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? _g_cur_win->GZZWidth : _g_cur_win->Width - _g_cur_win->BorderLeft - _g_cur_win->BorderRight;
    WORD win_height = _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? _g_cur_win->GZZHeight : _g_cur_win->Height - _g_cur_win->BorderTop - _g_cur_win->BorderBottom;
    WORD max_lines  = win_height / _g_cur_rp->Font->tf_YSize - 1;
    WORD cy         = _g_cur_rp->cp_y / _g_cur_rp->Font->tf_YSize;
    WORD scroll_y   = cy - max_lines;

    if (scroll_y > 0)
    {
        scroll_y *= _g_cur_rp->Font->tf_YSize;
        ScrollRaster (_g_cur_rp, win_x, win_y+scroll_y, win_x, win_y, win_width, win_height);
        Move (_g_cur_rp, _g_cur_rp->cp_x, _g_cur_rp->cp_y-scroll_y);
    }
}

static BOOL _awindow_puts(UBYTE *s)
{
    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {
        ULONG l = LEN_(s);
        Write(g_stdout, (CONST APTR) s, l);
        return TRUE;
    }

    // do a crude terminal emulation, reacting to control characters

    WORD startpos = 0;
    WORD pos = 0;

    while (TRUE)
    {
        char ch = s[pos];

        switch (ch)
        {
            case 0:
            case 7:
            case 8:
            case 9:
            case 10:
            case 12:
            case 13:
            {
                // this is a control character - print text so far first, then act on it
                SHORT length = pos-startpos;
                if (length > 0)
                    Text (_g_cur_rp, (UBYTE *) &s[startpos], length);

                switch (ch)
                {
                    case 0:         // string end marker
                        goto fini;

                    case 7:         // bell
                        DisplayBeep(NULL);
                        break;

                    case 8:         // backspace
                        if (_g_cur_rp->cp_x >= _g_cur_rp->Font->tf_XSize)
                        {
                            Move (_g_cur_rp, _g_cur_rp->cp_x-_g_cur_rp->Font->tf_XSize, _g_cur_rp->cp_y);
                            Text (_g_cur_rp, (UBYTE*) " ", 1);
                            Move (_g_cur_rp, _g_cur_rp->cp_x-_g_cur_rp->Font->tf_XSize, _g_cur_rp->cp_y);
                        }
                        break;

                    case 9:         // tab
                    {
                        int cx = _g_cur_rp->cp_x / _g_cur_rp->Font->tf_XSize;           // cursor position in nominal characters
                        cx = cx + (8-(cx%8));                                           // AmigaBASIC TABs are 9 characters wide
                        Move (_g_cur_rp, cx * _g_cur_rp->Font->tf_XSize, _g_cur_rp->cp_y);
                        break;
                    }
                    case 10:        // linefeed
                        Move (_g_cur_rp, 0, _g_cur_rp->cp_y + _g_cur_rp->Font->tf_YSize);
                        do_scroll();
                        break;

                    case 12:        // clear screen
                        Move (_g_cur_rp, 0, 0);
                        SetRast (_g_cur_rp, _g_cur_rp->BgPen);
                        Move (_g_cur_rp, 0, _g_cur_rp->Font->tf_Baseline);
                        break;

                    case 13:        // carriage return
                        Move (_g_cur_rp, 0, _g_cur_rp->cp_y);
                        break;
                }

                pos++;
                startpos = pos;
                break;
            }
            default:
                pos++;
        }

    }
    fini:
        return TRUE;
}

#define CSI 0x9b

static BOOL _awindow_locate (SHORT l, SHORT c)
{
    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {

        UBYTE buf[20];
        buf[0] = CSI;
        _astr_itoa_ext(l, &buf[1], 10, /*leading_space=*/FALSE);
        int l = LEN_(buf);
        buf[l] = ';';
        l++;
        _astr_itoa_ext(c, &buf[l], 10, /*leading_space=*/FALSE);
        l = LEN_(buf);
        buf[l] = 'H';
        buf[l+1] = 0;

        Write(g_stdout, (CONST APTR) buf, l+1);
        return TRUE;
    }

    if (l<=0)
        l = CSRLIN_();
    if (c<=0)
        c = POS_(0);

    l--;
    c--;

    WORD win_x = _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? 0 : _g_cur_win->BorderLeft;
    WORD win_y = _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? 0 : _g_cur_win->BorderTop;

    Move (_g_cur_rp, win_x + c * _g_cur_rp->Font->tf_XSize, win_y + l * _g_cur_rp->Font->tf_YSize + _g_cur_rp->Font->tf_Baseline);

    return TRUE;
}

void LOCATE_XY (BOOL s, SHORT x, SHORT y)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    if (s)
    {
        x += _g_cur_rp->cp_x;
        y += _g_cur_rp->cp_y;
    }

    Move (_g_cur_rp, x, y);
}

SHORT CSRLIN_ (void)
{
    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
        return 0;

    return _g_cur_rp->cp_y / _g_cur_rp->Font->tf_YSize + 1;
}

SHORT POS_ (SHORT dummy)
{
    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
        return 0;

    return _g_cur_rp->cp_x / _g_cur_rp->Font->tf_XSize + 1;
}

// input statement support

static void draw_cursor(void)
{
    BYTE   old_fg, fg;
    WORD   old_x, old_y;

    old_fg = _g_cur_rp->FgPen;
    old_x  = _g_cur_rp->cp_x;
    old_y  = _g_cur_rp->cp_y;

    fg = (1<<_g_cur_rp->BitMap->Depth)-1;

    DPRINTF ("draw_cursor: old_fg=%d old_x=%d old_y=%d fg=%d\n", old_fg, old_x, old_y, fg);

    SetAPen (_g_cur_rp, fg);
    RectFill (_g_cur_rp, old_x+1, old_y - _g_cur_rp->TxBaseline,
                         old_x+2, old_y - _g_cur_rp->TxBaseline + _g_cur_rp->TxHeight - 1);

    Move (_g_cur_rp, old_x, old_y);
    SetAPen (_g_cur_rp, old_fg);
}

static BOOL is_eol (UBYTE c)
{
    return (c=='\r') || (c=='\n');
}

static BOOL _awindow_gets(UBYTE *buf, USHORT buf_len, BOOL do_nl)
{
    static UBYTE twospaces[] = "  ";

    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {
        _aio_init();
        _AIO_SET_DOS_CURSOR_VISIBLE (TRUE);
        LONG bytes = Read(g_stdin, (CONST APTR) buf, buf_len);
        buf[bytes-1] = '\0';
        _AIO_SET_DOS_CURSOR_VISIBLE (FALSE);
    }
    else
    {
        // do a crude terminal emulation, handle backspace

        long col = 0;
        draw_cursor();

        while (TRUE)
        {
            //DPRINTF ("_aio_gets: sleep...\n");
            //CHKBRK;
            SLEEP();
            //DPRINTF ("_aio_gets: sleep... returned.\n");

            char *buf2 = INKEY_();
            char c = *buf2;

            if (c == '\0')
                continue;

            if (c != '\b')
            {
                if (is_eol(c))
                    break;

                Text (_g_cur_rp, (UBYTE *) buf2, 1);
                draw_cursor();

                buf[col] = c;
                col++;
            }
            else
            {
                if (col > 0)
                {
                    int x = _g_cur_rp->cp_x - _g_cur_rp->Font->tf_XSize;
                    int y = _g_cur_rp->cp_y;

                    // erase last char + cursor
                    Move(_g_cur_rp, x, y);
                    Text(_g_cur_rp, (UBYTE*) &twospaces, 2);

                    // draw new cursor
                    Move(_g_cur_rp, x, y);
                    draw_cursor();

                    col--;
                }
            }
        }

        buf[col] = '\0';

        // cleanup cursor
        Text (_g_cur_rp, (UBYTE *) &twospaces, 1);

        if (do_nl)
        {
            //_debug_puts((STRPTR)"do_nl");
            Move (_g_cur_rp, 0, _g_cur_rp->cp_y + _g_cur_rp->Font->tf_YSize);
            do_scroll();
        }
        else
        {
            //_debug_puts((STRPTR)"NOT do_nl");
        }
    }

#ifdef ENABLE_DEBUG
    DPRINTF ("_aio_gets: buf=%s\n", buf);
#endif

    return TRUE;
}


/*******************************************************************
 *
 * drawing functions
 *
 *******************************************************************/

void PALETTE(short cid, FLOAT red, FLOAT green, FLOAT blue)
{
    if (!_g_cur_vp)
    {
        ERROR(AE_PALETTE);
        return;
    }

    if ( (cid < 0) || (cid >63) )
    {
        ERROR(AE_PALETTE);
        return;
    }

    LONG r = SPFix(SPMul(red, g_fp15));
    if ((r<0) || (r>15))
    {
        ERROR(AE_PALETTE);
        return;
    }

    LONG g = SPFix(SPMul(green, g_fp15));
    if ((g<0) || (g>15))
    {
        ERROR(AE_PALETTE);
        return;
    }

    LONG b = SPFix(SPMul(blue, g_fp15));
    if ((b<0) || (b>15))
    {
        ERROR(AE_PALETTE);
        return;
    }

    SetRGB4(_g_cur_vp, cid, r, g, b);
}

void COLOR(short fg, short bg, short o, short drmd)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    if (fg >= 0)
        SetAPen (_g_cur_rp, fg);
    if (bg >= 0)
        SetBPen (_g_cur_rp, bg);
    if (o >= 0)
        _g_cur_rp->AOlPen = o;
    if (drmd >=0)
        SetDrMd (_g_cur_rp, drmd);
}

static void allocTmpRas(void)
{
    struct TmpRas *aTmpRas = AllocVec(sizeof(*aTmpRas), MEMF_CLEAR);
    if (!aTmpRas)
    {
        ERROR(AE_PAINT);
        return;
    }

#ifdef ENABLE_DEBUG
    _debug_puts((STRPTR)"allocTmpRas: AllocVec aTmpRas ->"); _debug_putu4((ULONG)aTmpRas); _debug_putnl();
#endif
    ULONG rassize = RASSIZE (_g_cur_win->Width, _g_cur_win->Height);

    //_debug_puts((STRPTR)"allocTmpRas: rassize="); _debug_putu4(rassize);
    //_debug_putnl();

    PLANEPTR amem = AllocVec (rassize, MEMF_CHIP|MEMF_CLEAR);
    if (!amem)
    {
        ERROR(AE_PAINT);
        return;
    }
#ifdef ENABLE_DEBUG
    _debug_puts((STRPTR)"allocTmpRas: AllocVec amem ->"); _debug_putu4((ULONG)amem); _debug_putnl();
#endif
    InitTmpRas (aTmpRas, amem, rassize);
    _g_cur_rp->TmpRas = aTmpRas;
}

void PAINT(BOOL s, short x, short y, short pc, short aol)
{
   _aqb_get_output (/*needGfx=*/TRUE);

    // init tmp raster if not done yet
    if (!_g_cur_rp->TmpRas)
        allocTmpRas();

    if (s)
    {
        x += _g_cur_rp->cp_x;
        y += _g_cur_rp->cp_y;
    }

    BYTE fgPen=_g_cur_rp->FgPen;
    BYTE aolPen=_g_cur_rp->AOlPen;
    if (pc >= 0)
        SetAPen(_g_cur_rp, pc);
    if (aol >= 0)
        _g_cur_rp->AOlPen = aol;

    Flood (_g_cur_rp, 0, x, y);

    if ( pc >=0 )
        SetAPen(_g_cur_rp, fgPen);
    if ( aol >= 0 )
        _g_cur_rp->AOlPen = aolPen;
}

#define AREA_MAX_CNT    100

void AREA(BOOL s, short x, short y)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    // first call on this rp? -> initialize Area* related data structures

    if (!_g_cur_rp->AreaInfo)
    {
        struct AreaInfo *ai = ALLOCATE_(sizeof (*ai), 0);
        if (!ai)
        {
            ERROR(AE_AREA);
            return;
        }
        APTR adata = ALLOCATE_(AREA_MAX_CNT*5, 0);
        if (!adata)
        {
            ERROR(AE_AREA);
            return;
        }
        InitArea(ai, adata, AREA_MAX_CNT);
        _g_cur_rp->AreaInfo = ai;
    }

    // init tmp raster if not done yet
    if (!_g_cur_rp->TmpRas)
        allocTmpRas();

    if (s)
    {
        x += _g_cur_rp->cp_x;
        y += _g_cur_rp->cp_y;
    }

    WORD cnt = _g_cur_rp->AreaInfo->Count;

    if (cnt >= AREA_MAX_CNT)
    {
        ERROR(AE_AREA);
        return;
    }

    if (cnt==0)
        AreaMove (_g_cur_rp, x, y);
    else
        AreaDraw (_g_cur_rp, x, y);
}

void AREAFILL (short mode)
{
    BYTE dm;

    _aqb_get_output (/*needGfx=*/TRUE);

    dm = _g_cur_rp->DrawMode;
    if (mode==1)
    {
        SetDrMd(_g_cur_rp, COMPLEMENT);
    }
    else
    {
        SetDrMd(_g_cur_rp, JAM2);
    }

    AreaEnd(_g_cur_rp);

    SetDrMd(_g_cur_rp, dm);
}

void AREA_OUTLINE(BOOL enabled)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    if (enabled)
        _g_cur_rp->Flags |= AREAOUTLINE;
    else
        _g_cur_rp->Flags &= ~AREAOUTLINE;
}

void PATTERN (unsigned short lineptrn, CArray *areaptrn)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    _g_cur_rp->LinePtrn   = lineptrn;
    _g_cur_rp->Flags     |= FRST_DOT;
    _g_cur_rp->linpatcnt  = 15;

    if (areaptrn)
    {
        if (areaptrn->_numDims != 1)
        {
            ERROR(AE_PATTERN);
            return;
        }

        ULONG n = areaptrn->_bounds[0].ubound - areaptrn->_bounds[0].lbound + 1;
        //_debug_puts((STRPTR)"PATTERN area: n="); _debug_puts2(n);

        // log2
        ULONG ptSz = 0;
        while (n >>= 1) ++ptSz;

        //_debug_puts((STRPTR)", ptSz="); _debug_puts2(ptSz); _debug_putnl();
        //_debug_puts((STRPTR)"AreaPtrn[0]="); _debug_putu4(*((ULONG*)areaptrn->data)); _debug_putnl();

        _g_cur_rp->AreaPtrn = (UWORD *) areaptrn->_data;
        _g_cur_rp->AreaPtSz = ptSz;
    }
}

void PATTERN_RESTORE (void)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    _g_cur_rp->LinePtrn   = 0xFFFF;
    _g_cur_rp->Flags     |= FRST_DOT;
    _g_cur_rp->linpatcnt  = 15;
    _g_cur_rp->AreaPtrn   = NULL;
    _g_cur_rp->AreaPtSz   = 0;
}

void BITMAP_FREE (BITMAP_t *bm)
{
    if (bm->prev)
        bm->prev->next = bm->next;
    else
        g_bm_first = bm->next;

    if (bm->next)
	{
        bm->next->prev = bm->prev;
	}
    else
	{
        g_bm_last = bm->prev;
	}

    if (bm->mask)
    {
        FreeVec(bm->mask);
        bm->mask = NULL;
    }

	if (bm->continous)
	{
	   FreeVec(bm->bm.Planes[0]);
	   bm->bm.Planes[0] = NULL;
	}
	else
	{
		for (SHORT plane_num = 0; plane_num < bm->bm.Depth; plane_num++)
		{
		   FreeRaster(bm->bm.Planes[plane_num], bm->width, bm->height);
		   bm->bm.Planes[plane_num] = NULL;
		}
	}

    FreeVec (bm);
}

BITMAP_t *BITMAP_ (SHORT width, SHORT height, SHORT depth, BOOL cont)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("BITMAP_: allocating new bitmap, width=%d, height=%d, depth=%d, cont=%d\n", width, height, depth, cont);
#endif

    BITMAP_t *bm = AllocVec(sizeof(*bm), MEMF_CLEAR);
    if (!bm)
    {
        ERROR(AE_BLIT);
        return NULL;
    }

    bm->prev = g_bm_last;
    if (g_bm_last)
        g_bm_last = g_bm_last->next = bm;
    else
        g_bm_first = g_bm_last = bm;

    bm->width     = width;
    bm->height    = height;
	bm->continous = cont;
    bm->mask      = NULL;

    InitBitMap(&bm->bm, depth, width, height);

	if (cont)
	{
		ULONG rs = RASSIZE (width, height);
		BYTE *p = AllocVec (rs * depth, MEMF_CHIP | MEMF_CLEAR);
		if (!p)
		{
#ifdef ENABLE_DEBUG
			DPRINTF ("BITMAP_: continous plane alloc of %d bytes failed\n", rs * depth);
#endif
			ERROR(AE_BLIT);
			return NULL;
		}
		for (SHORT plane_num = 0; plane_num < depth; plane_num++)
		{
			bm->bm.Planes[plane_num] = (PLANEPTR)p;
			p += rs;
		}
	}
	else
	{
		for (SHORT plane_num = 0; plane_num < depth; plane_num++)
		{
			bm->bm.Planes[plane_num] = (PLANEPTR)AllocRaster(width, height);
			if (!bm->bm.Planes[plane_num])
			{
				ERROR(AE_BLIT);
				return NULL;
			}
		}
	}

    InitRastPort (&bm->rp);
    bm->rp.BitMap = &bm->bm;

    return bm;
}

void BITMAP_OUTPUT (BITMAP_t *bm)
{
    if (!bm)
    {
        ERROR(AE_BLIT);
        return;
    }
    _g_cur_bm       = bm;
    _g_cur_rp       = &bm->rp;
    g_cur_ot        = _aqb_ot_bitmap;
}

static void _bitmap_compute_mask (BITMAP_t *bm)
{
    ULONG rs = RASSIZE (bm->width, bm->height);
    UWORD *dst = (UWORD*) bm->mask;
    memset (dst, 0, rs);
    for (UWORD d=0; d<bm->bm.Depth; d++)
    {
        dst = (UWORD*)bm->mask;
        UWORD *src = (UWORD*)bm->bm.Planes[d];
        for (ULONG i=0; i<rs/2; i++)
            *dst++ |= *src++;
    }
}

void BITMAP_MASK (BITMAP_t *bm)
{
    if (!bm)
    {
        ERROR(AE_BLIT);
        return;
    }

    ULONG rs = RASSIZE (bm->width, bm->height);
    if (!bm->mask)
    {
        bm->mask = AllocVec (rs, MEMF_CHIP);
        if (!bm->mask)
        {
            ERROR(AE_BLIT);
            return;
        }
    }

    _bitmap_compute_mask (bm);
}

void GET (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, BITMAP_t *bm)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    if (x1<0)
        x1 = _g_cur_rp->cp_x;
    if (y1<0)
        y1 = _g_cur_rp->cp_y;

    if (s1)
    {
        x1 += _g_cur_rp->cp_x;
        y1 += _g_cur_rp->cp_y;
    }
    if (s2)
    {
        x2 += _g_cur_rp->cp_x;
        y2 += _g_cur_rp->cp_y;
    }

    SHORT w = x2-x1+1;
    if (w<=0)
    {
        ERROR(AE_BLIT);
        return;
    }
    SHORT h = y2-y1+1;
    if (h<=0)
    {
        ERROR(AE_BLIT);
        return;
    }

    ClipBlit(_g_cur_rp, x1, y1, &bm->rp, 0, 0, w, h, 0xC0);
    if (bm->mask)
        _bitmap_compute_mask (bm);
}

void PUT (BOOL s, SHORT x, SHORT y, BITMAP_t *bm, UBYTE minterm, BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    if ( s1 || s2 )
    {
        ERROR(AE_BLIT);
        return;
    }
    if (x<0)
        x = _g_cur_rp->cp_x;
    if (y<0)
        y = _g_cur_rp->cp_y;

    if (s)
    {
        x += _g_cur_rp->cp_x;
        y += _g_cur_rp->cp_y;
    }

    if (x1<0) x1 = 0;
    if (y1<0) y1 = 0;
    if (x2<0) x2 = bm->width-1;
    if (y2<0) y2 = bm->height-1;

    SHORT w = x2-x1+1;
    if (w<=0)
    {
        ERROR(AE_BLIT);
        return;
    }
    SHORT h = y2-y1+1;
    if (h<=0)
    {
        ERROR(AE_BLIT);
        return;
    }

    if (bm->mask)
        BltMaskBitMapRastPort (&bm->bm, x1, y1, _g_cur_rp, x, y, w, h, minterm, bm->mask);
    else
        ClipBlit(&bm->rp, x1, y1, _g_cur_rp, x, y, w, h, minterm);
}

static struct DiskFontHeader *_loadFont (char *font_path)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("_loadFont: font_path: %s\n", font_path);
#endif

    BPTR seglist = LoadSeg ((STRPTR)font_path);

    if (seglist)
    {
        struct DiskFontHeader *dfh;

        dfh = (struct DiskFontHeader *) (BADDR(seglist) + 8);
        dfh->dfh_Segment = seglist;

        return dfh;
    }
    else
    {
        DPRINTF("_loadFont: LoadSeg failed!\n");
    }

    return NULL;
}

static void _freeFont (struct DiskFontHeader *dfh)
{
    BPTR seglist = dfh->dfh_Segment;
    if (!seglist)
        return;
    dfh->dfh_Segment = 0l;
    UnLoadSeg (seglist);
}

void FONT_FREE (FONT_t *font)
{
    if (font->prev)
        font->prev->next = font->next;
    else
        g_font_first = font->next;

    if (font->next)
	{
        font->next->prev = font->prev;
	}
    else
	{
        g_font_last = font->prev;
	}

    if (font->tf)
        CloseFont(font->tf);

    if (font->dfh)
        _freeFont (font->dfh);

    FreeVec (font);
}

FONT_t *FONT_ (UBYTE *font_name, SHORT font_size, UBYTE *font_dir)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("FONT_: allocating new font, font_name=%s, font_size=%d\n", font_name, font_size);
    DPRINTF ("                            font_dir=%s\n", font_dir ? (char *)font_dir : "NULL");
#endif

	struct TextFont *tf = NULL;
	struct DiskFontHeader *dfh = NULL;
	if (font_dir)
	{
		static char font_path[256];
		static char str_font_size[10];

		_astr_itoa_ext (font_size, (STRPTR) str_font_size, 10, /*leading_space=*/FALSE);

		ULONG l = LEN_(font_dir);
		ULONG l2 = LEN_(font_name);
		if (l+l2>254)
        {
            DPRINTF ("FONT_: font path overflow\n");
			ERROR(AE_FONT);
			return NULL;
        }
		CopyMem ((APTR)font_dir, (APTR)font_path, l+1);
		AddPart ((STRPTR) font_path, (STRPTR)font_name, 256);
        // remove .font suffix
		l = LEN_((STRPTR) font_path);
        if (l<6)
        {
            DPRINTF ("FONT_: font name is too short\n");
			ERROR(AE_FONT);
			return NULL;
        }
        font_path[l-5]=0;
		AddPart ((STRPTR) font_path, (STRPTR)str_font_size, 256);

		dfh = _loadFont (font_path);

		if (!dfh)
		{
            DPRINTF ("FONT_: _dfh==NULL\n");
			ERROR(AE_FONT);
			return NULL;
		}

		tf = &dfh->dfh_TF;
	}
	else
	{
		struct TextAttr textattr = {(STRPTR)font_name, font_size, 0, 0};

		tf = OpenDiskFont (&textattr);
		if (!tf)
		{
            DPRINTF ("FONT_: OpenDiskFont failed\n");
			ERROR(AE_FONT);
			return NULL;
		}
	}


    FONT_t *font = AllocVec(sizeof(*font), MEMF_CLEAR);
    if (!font)
    {
        ERROR(AE_FONT);
        return NULL;
    }

    font->prev = g_font_last;
    if (g_font_last)
	{
        g_font_last = g_font_last->next = font;
	}
    else
	{
        g_font_first = g_font_last = font;
	}

	font->dfh = dfh;
	font->tf  = tf;

    return font;
}

void FONT (FONT_t *font)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    SetFont (_g_cur_rp, font->tf);
}

void FONTSTYLE (ULONG style)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    SetSoftStyle (_g_cur_rp, style, 0xFF);
}

ULONG FONTSTYLE_ (void)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    return AskSoftStyle (_g_cur_rp);
}

SHORT TEXTWIDTH_ (UBYTE *s)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    SHORT l = LEN_(s);
    return TextLength (_g_cur_rp, s, l);
}

void TEXTEXTEND (UBYTE *s, SHORT *w, SHORT *h)
{
    _aqb_get_output (/*needGfx=*/TRUE);
    SHORT l = LEN_(s);
    struct TextExtent te;
    TextExtent (_g_cur_rp, s, l, &te);
    *w = te.te_Width;
    *h = te.te_Height;
}

void _palette_load (SHORT scid, PALETTE_t *p)
{
    struct Screen *scr = g_scrlist[scid-1];
    if (!scr)
    {
        ERROR(AE_PALETTE);
        return;
    }

    for (SHORT cid = 0; cid<p->numEntries; cid++)
    {
        //DPRINTF("PALETTE_LOAD: entry #%d: %d/%d/%d\n", cid, p->colors[cid].r, p->colors[cid].g, p->colors[cid].b);
        SetRGB4(&scr->ViewPort, cid, p->colors[cid].r/16, p->colors[cid].g/16, p->colors[cid].b/16);
    }
}

void PALETTE_LOAD (PALETTE_t *p)
{
    if (!_g_cur_scr)
    {
        ERROR(AE_PALETTE);
        return;
    }
    _palette_load (_g_active_scr_id, p);
}

static char inkeybuf[2] = { 0, 0 } ;

char *INKEY_ (void)
{
    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {
        LONG l = Read(g_stdin, (CONST APTR) inkeybuf, 1);
        if (l != 1)
            return "";
        return inkeybuf;
    }

    if (!keybuf_len)
    {
        inkeybuf[0] = 0;
        return inkeybuf;
    }

    inkeybuf[0] = keybuf[keybuf_start];
    keybuf_start = (keybuf_start + 1) % MAXKEYBUF;
    keybuf_len--;

    return inkeybuf;
}

void _awindow_init(void)
{
    g_stdout = Output();
    g_stdin  = Input();

    _aio_puts_cb        = _awindow_puts;
    _aio_gets_cb        = _awindow_gets;
    _aio_cls_cb         = _awindow_cls;
    _aio_locate_cb      = _awindow_locate;
    _autil_sleep_for_cb = _awindow_sleep_for;

    for (int i =0; i<MAX_NUM_WINDOWS; i++)
    {
        // DPRINTF ("awindow initializing, i=%d _g_winlist[i]=0x%08lx, sizeof(_g_winlist)=%ld\n", i, &_g_winlist[i], sizeof (_g_winlist));
        _g_winlist[i].win            = NULL;
        _g_winlist[i].aqb_close_cb   = NULL;
        _g_winlist[i].aqb_close_ud   = NULL;
        _g_winlist[i].aqb_newsize_cb = NULL;
        _g_winlist[i].aqb_newsize_ud = NULL;
        _g_winlist[i].aqb_refresh_cb = NULL;
        _g_winlist[i].aqb_refresh_ud = NULL;
        _g_winlist[i].close_cbs      = NULL;
        _g_winlist[i].msg_cbs        = NULL;
    }

    g_fp15   = SPFlt(15);
    g_fp50   = SPFlt(50);

    if (0 == OpenDevice((STRPTR)"console.device", -1, (struct IORequest *)&g_ioreq, 0))
    {
        g_console_device_opened=TRUE;
        ConsoleDevice = g_ioreq.io_Device;
    }

    g_cur_ot = _startup_mode == STARTUP_CLI ? _aqb_ot_console : _aqb_ot_none;

	// default view port

	struct Screen *sc = NULL;
	if ( (sc = LockPubScreen(NULL)) )
	{
		_g_cur_vp = &sc->ViewPort;
		UnlockPubScreen(NULL, sc);
	}
}

