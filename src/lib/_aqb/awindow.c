#include "_aqb.h"
#include "../_brt/_brt.h"

#include <stdarg.h>

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

//#define ENABLE_DEBUG

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

struct Device           *ConsoleDevice;
BPTR                     g_stdout, g_stdin;
static FLOAT             g_fp15; // FFP representation of decimal 15, used in PALETTE
static FLOAT             g_fp50; // FFP representation of decimal 50, used in SLEEP_FOR
static struct IOStdReq   g_ioreq; // console.device is used to convert RAWKEY codes
static BOOL              g_console_device_opened=FALSE;
static struct InputEvent g_ievent;

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

#define MAX_NUM_WINDOWS 16

static struct Window * g_winlist[MAX_NUM_WINDOWS] = {
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL
};

// window callback

typedef struct win_cb_node_s *win_cb_node_t;
struct win_cb_node_s
{
    win_cb_node_t       next;
    window_close_cb_t   cb;
};

static win_cb_node_t g_win_cb_list[MAX_NUM_WINDOWS] = {
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL
};

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

static ULONG                 _g_signalmask_awindow=0;

static BITMAP_t             *g_bm_first      = NULL;
static BITMAP_t             *g_bm_last       = NULL;

static void (*g_win_cb)(void)                = NULL;
static void (*g_mouse_cb)(void)              = NULL;
static void (*g_mouse_motion_cb)(void)       = NULL;

static short                 g_active_scr_id = 0;
static short                 g_active_win_id = 1;
static short                 g_output_win_id = 1;

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

    g_scrlist[id-1] = scr;
    _g_cur_scr      = scr;
    g_active_scr_id = id;
	_g_cur_vp       = &scr->ViewPort;
    g_cur_ot        = _aqb_ot_screen;
    _g_cur_rp       = &scr->RastPort;
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
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (g_winlist[id-1] != NULL) || (x1 > x2) || (y1 > y2) )
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
    g_nw.IDCMPFlags = CLOSEWINDOW | RAWKEY | ACTIVEWINDOW; // INTUITICKS | VANILLAKEY | MENUPICK | GADGETUP | ACTIVEWINDOW;

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

    struct Window *win = (struct Window *)OpenWindow(&g_nw);

    if (!win)
    {
        ERROR(AE_WIN_OPEN);
        return;
    }

    g_winlist[id-1] = win;

    _g_signalmask_awindow |= (1L << win->UserPort->mp_SigBit);

    _g_cur_win      = win;
    _g_cur_rp       = win->RPort;
    g_output_win_id = id;
    g_cur_ot        = _aqb_ot_window;

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
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (g_winlist[id-1] == NULL) )
    {
        ERROR(AE_WIN_CLOSE);
        return;
    }

    // call close callbacks first
    for (win_cb_node_t n=g_win_cb_list[id-1]; n; n=n->next)
    {
#ifdef ENABLE_DEBUG
        DPRINTF ("WINDOW_CLOSE calling close cb 0x%08lx)\n", n->cb);
#endif
        n->cb(g_winlist[id-1]);
    }

    if (g_winlist[id-1]->RPort->TmpRas)
    {
        FreeVec ((PLANEPTR) g_winlist[id-1]->RPort->TmpRas->RasPtr);
        FreeVec (g_winlist[id-1]->RPort->TmpRas);
    }
#ifdef ENABLE_DEBUG
    DPRINTF ("WINDOW_CLOSE id=%d CloseWindow(0x%08lx)\n", id, g_winlist[id-1]);
#endif
    CloseWindow(g_winlist[id-1]);
    g_winlist[id-1]=NULL;
}

void _window_add_close_cb (window_close_cb_t cb)
{
#ifdef ENABLE_DEBUG
    DPRINTF ("_window_add_close_cb g_active_win_id=%d, cb=0x%08lx\n", g_active_win_id, cb);
#endif
    win_cb_node_t node = ALLOCATE_(sizeof (*node), 0);
    if (!node)
    {
        ERROR (AE_WIN_CLOSE);
        return;
    }
    node->next = g_win_cb_list[g_active_win_id-1];
    node->cb   = cb;
    g_win_cb_list[g_active_win_id-1] = node;
}

/*
 * WINDOW OUTPUT id
 */
void WINDOW_OUTPUT(short id)
{
    // switch (back) to console output?
    if ( (id == 1) && !g_winlist[0] && (_startup_mode == STARTUP_CLI) )
    {
        g_output_win_id = 1;
        _g_cur_win      = NULL;
        _g_cur_rp       = NULL;
        g_cur_ot        = _aqb_ot_console;
        return;
    }

    // error checking
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (g_winlist[id-1] == NULL) )
    {
        ERROR(AE_WIN_OUTPUT);
        return;
    }

    g_output_win_id = id;

    struct Window *win = g_winlist[id-1];

    _g_cur_win      = win;
    _g_cur_rp       = win->RPort;
    g_cur_ot        = _aqb_ot_window;
}

void _awindow_shutdown(void)
{
#ifdef ENABLE_DEBUG
    _debug_puts((STRPTR)"_awindow_shutdown ...\n");
#endif
    for (int i = 0; i<MAX_NUM_WINDOWS; i++)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_awindow_shutdown g_winlist[%d]=0x%08lx\n", i, g_winlist[i]);
#endif
        if (g_winlist[i])
            WINDOW_CLOSE(i+1);
    }
    for (int i = 0; i<MAX_NUM_SCREENS; i++)
    {
        if (g_scrlist[i])
        {
#ifdef ENABLE_DEBUG
            _debug_puts((STRPTR)"_awindow_shutdown ... CloseScreen\n");
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

    _aio_set_dos_cursor_visible (TRUE);
    if (g_console_device_opened)
    {
#ifdef ENABLE_DEBUG
        _debug_puts((STRPTR)"_awindow_shutdown ... CloseDevice\n");
        //Delay (100);
#endif
        CloseDevice((struct IORequest *)&g_ioreq);
    }
#ifdef ENABLE_DEBUG
    _debug_puts((STRPTR)"_awindow_shutdown ... finished\n");
    //Delay (100);
#endif
}

void _awindow_init(void)
{
    g_stdout = Output();
    g_stdin  = Input();
    g_fp15   = SPFlt(15);
    g_fp50   = SPFlt(50);
    _aio_set_dos_cursor_visible (FALSE);
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

/*
 * CLS
 */

void CLS (void)
{
    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {
        char form_feed = 0x0c;
        Write(g_stdout, (CONST APTR) &form_feed, 1);
        return;
    }

    Move (_g_cur_rp, 0, 0);
    ClearScreen(_g_cur_rp);
    LOCATE(1, 1);
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
#endif

    if (signals & _g_signalmask_atimer)
        _atimer_process_signals(signals);

    for (int i =0; i<MAX_NUM_WINDOWS; i++)
    {
        struct Window *win = g_winlist[i];
        if (!win)
            continue;

        if (!(signals & (1L << win->UserPort->mp_SigBit)) )
            continue;

        while ( (message = (struct IntuiMessage *)GetMsg(win->UserPort) ) )
        {
            ULONG class = message->Class;

#ifdef ENABLE_DEBUG
            DPRINTF("_handleSignals: got a message, class=0x%08lx\n", class);
#endif

            switch(class)
            {
                case CLOSEWINDOW:
                    if (g_win_cb)
                    {
                        g_win_cb();
                    }
                    break;

                case MOUSEBUTTONS:
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
							g_mouse_down_x = message->MouseX;
							g_mouse_down_y = message->MouseY;
							break;
						case SELECTUP:
							g_mouse_bev  = TRUE;
							g_mouse_down = FALSE;
							g_mouse_up_x = message->MouseX;
							g_mouse_up_y = message->MouseY;
							break;
					}
 
                    if (g_mouse_cb)
                    {
                        g_mouse_cb();
                    }
                    break;

                case MOUSEMOVE:
                    if (g_mouse_motion_cb)
                    {
                        g_mouse_motion_cb();
                    }
                    break;

                case ACTIVEWINDOW:
                    g_active_win_id = i+1;
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
            }

            ReplyMsg ( (struct Message *) message);
            //_debug_puts((STRPTR)"sleep: replied.\n");
        }
    }
}

void SLEEP(void)
{
	_handleSignals(/*doWait=*/TRUE);
}

void SLEEP_FOR (FLOAT s)
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

void ON_WINDOW_CALL(void (*cb)(void))
{
    g_win_cb = cb;
}

ULONG WINDOW_(short n)
{
    switch(n)
    {
        case 0:                                 //  0: current active window
            return g_active_win_id;
        case 1:                                 //  1: current output window id
            return g_output_win_id;
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

void ON_MOUSE_CALL (void (*cb)(void))
{
    g_mouse_cb = cb;
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
            return _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? _g_cur_win->GZZMouseX : _g_cur_win->MouseX;
        case 2:
            return _g_cur_win->Flags & WFLG_GIMMEZEROZERO ? _g_cur_win->GZZMouseY : _g_cur_win->MouseY;

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

void ON_MOUSE_MOTION_CALL (void (*cb)(void))
{
    g_mouse_motion_cb = cb;
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

void _aio_puts(USHORT fno, const UBYTE *s)
{
    //_debug_puts((STRPTR)"_debug_puts\n");

    if (fno)
    {
        _aio_fputs (fno, s);
        return;
    }

    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {
        //_debug_puts((STRPTR)"_debug_puts: stdout\n");
        ULONG l = LEN_(s);
        //_debug_puts((STRPTR)"_debug_puts: l=");_debug_putu4(l); _debug_putnl();
        //_debug_puts((STRPTR)"_debug_puts: g_stdout=");_debug_putu4((ULONG) g_stdout); _debug_putnl();
        Write(g_stdout, (CONST APTR) s, l);
        return;
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
                        int cx = _g_cur_rp->cp_x / _g_cur_rp->Font->tf_XSize;          // cursor position in nominal characters
                        // _debug_puts((STRPTR)"[1] cx="); _debug_puts2(cx); _debug_puts((STRPTR)"\n");
                        cx = cx + (8-(cx%8));                                // AmigaBASIC TABs are 9 characters wide
                        // _debug_puts((STRPTR)"[2] cx="); _debug_puts2(cx); _debug_puts((STRPTR)"\n");
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
        return;
}

void _aio_puttab(USHORT fno)
{
    if (fno)
    {
        _aio_fputs(fno, (STRPTR) "\t");
        return;
    }

    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {
        Write(g_stdout, (CONST APTR) "\t", 1);
        return;
    }

    int cx = _g_cur_rp->cp_x / _g_cur_rp->Font->tf_XSize;          // cursor position in nominal characters
    cx = cx + (14-(cx%14));                              // PRINT comma TABs are 15 characters wide
    Move (_g_cur_rp, cx * _g_cur_rp->Font->tf_XSize, _g_cur_rp->cp_y);
}

#define CSI 0x9b

void LOCATE (SHORT l, SHORT c)
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
        return;
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

void _aio_set_dos_cursor_visible (BOOL visible)
{
    static UBYTE csr_on[]   = { CSI, '1', ' ', 'p', '\0' };
    static UBYTE csr_off[]  = { CSI, '0', ' ', 'p', '\0' };

    UBYTE *c = visible ? csr_on : csr_off;
    Write(g_stdout, (CONST APTR) c, LEN_(c));
}

static void draw_cursor(void)
{
    CHKBRK;

    ULONG   old_fg, old_x, old_y;

    old_fg = _g_cur_rp->FgPen;
    old_x  = _g_cur_rp->cp_x;
    old_y  = _g_cur_rp->cp_y;

    SetAPen (_g_cur_rp, (1<<_g_cur_rp->BitMap->Depth)-1);
    RectFill (_g_cur_rp, old_x+1, old_y - _g_cur_rp->TxBaseline,
                    old_x+2, old_y - _g_cur_rp->TxBaseline + _g_cur_rp->TxHeight - 1);

    Move (_g_cur_rp, old_x, old_y);
    SetAPen (_g_cur_rp, old_fg);
}

#define MAXINPUTBUF 1024

static BOOL is_eol (UBYTE c)
{
    return (c=='\r') || (c=='\n');
}

void _aio_gets(UBYTE **s, BOOL do_nl)
{
    static UBYTE buf[MAXINPUTBUF];
    static UBYTE twospaces[] = "  ";

    if (_aqb_get_output (/*needGfx=*/FALSE) == _aqb_ot_console)
    {
        _aio_set_dos_cursor_visible (TRUE);
        LONG bytes = Read(g_stdin, (CONST APTR) buf, MAXINPUTBUF);
        buf[bytes-1] = '\0';
        _aio_set_dos_cursor_visible (FALSE);
    }
    else
    {

        // do a crude terminal emulation, handle backspace

        long col = 0;
        draw_cursor();

        while (TRUE)
        {
            DPRINTF ("_aio_gets: sleep...\n");
            SLEEP();
            DPRINTF ("_aio_gets: sleep... returned.\n");
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

    *s = _astr_dup (buf);

    return;
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

void PATTERN (unsigned short lineptrn, _DARRAY_T *areaptrn)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    _g_cur_rp->LinePtrn   = lineptrn;
    _g_cur_rp->Flags     |= FRST_DOT;
    _g_cur_rp->linpatcnt  = 15;

    if (areaptrn)
    {
        if (areaptrn->numDims != 1)
        {
            ERROR(AE_PATTERN);
            return;
        }

        ULONG n = areaptrn->bounds[0].ubound - areaptrn->bounds[0].lbound + 1;
        //_debug_puts((STRPTR)"PATTERN area: n="); _debug_puts2(n);

        // log2
        ULONG ptSz = 0;
        while (n >>= 1) ++ptSz;

        //_debug_puts((STRPTR)", ptSz="); _debug_puts2(ptSz); _debug_putnl();
        //_debug_puts((STRPTR)"AreaPtrn[0]="); _debug_putu4(*((ULONG*)areaptrn->data)); _debug_putnl();

        _g_cur_rp->AreaPtrn = areaptrn->data;
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

    bm->width  = width;
    bm->height = height;
	bm->continous = cont;

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

    ClipBlit(&bm->rp, x1, y1, _g_cur_rp, x, y, w, h, 0xC0);
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
    _palette_load (g_active_scr_id, p);
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

