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

#include <proto/console.h>
#include <clib/console_protos.h>
#include <pragmas/console_pragmas.h>

//#define ENABLE_DEBUG

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

static ULONG _g_signalmask_awindow=0;

/*
 * BitMap management
 */

typedef struct BitMapNode_ *BitMapNode;
struct BitMapNode_
{
    BitMapNode    *next;
    struct BitMap *bm;
};

//static BitMapNode g_bmn_first           = NULL;
//static BitMapNode g_bmn_last            = NULL;

static void (*g_win_cb)(void)           = NULL;
static void (*g_mouse_cb)(void)         = NULL;
static void (*g_mouse_motion_cb)(void)  = NULL;

static struct Screen   *g_active_scr    = NULL;
static short            g_active_scr_id = 0;
static short            g_active_win_id = 1;
static short            g_output_win_id = 1;
static struct Window   *g_output_win    = NULL;
static struct RastPort *g_rp            = NULL;
static BOOL             g_win1_is_dos   = TRUE; // when started from CLI, window 1 is the DOS stdout unless re-opened

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

void SCREEN (SHORT id, SHORT width, SHORT height, SHORT depth, SHORT mode, UBYTE *title)
{
    CHKBRK;

    // error checking
    if ( (id < 1) || (id > MAX_NUM_SCREENS) || (g_scrlist[id-1] != NULL) || (width <=0) || (height <= 0) || (depth <= 0) || (depth>6) )
    {
        ERROR(AE_SCREEN_OPEN);
        return;
    }

    //_debug_puts((STRPTR)"SCREEN title: "); _debug_puts(title); _debug_putnl();

    g_nscr.Width        = width;
    g_nscr.Height       = height;
    g_nscr.Depth        = depth;
    g_nscr.DefaultTitle = title ? (UBYTE *)_astr_dup(title) : (UBYTE*) "";
    g_nscr.ViewModes    = 0;

    switch (mode)
    {
        case AS_MODE_LORES:
            break;
        case AS_MODE_HIRES:
            g_nscr.ViewModes |= HIRES;
            break;
        case AS_MODE_LORES_LACED:
            g_nscr.ViewModes |= LACE;
            break;
        case AS_MODE_HIRES_LACED:
            g_nscr.ViewModes |= HIRES | LACE;
            break;
        case AS_MODE_HAM:
            g_nscr.ViewModes |= HAM;
            break;
        case AS_MODE_EXTRAHALFBRITE:
            g_nscr.ViewModes |= EXTRA_HALFBRITE;
            break;
        case AS_MODE_HAM_LACED:
            g_nscr.ViewModes |= HAM | LACE;
            break;
        case AS_MODE_EXTRAHALFBRITE_LACED:
            g_nscr.ViewModes |= EXTRA_HALFBRITE | LACE;
            break;
        default:
            ERROR(AE_SCREEN_OPEN);
            return;
    }

    // _debug_puts((STRPTR)"g_nscr.ViewModes:"); _debug_puts2(g_nscr.ViewModes); _debug_puts((STRPTR)"");

    struct Screen *scr = (struct Screen *)OpenScreen(&g_nscr);

    if (!scr)
    {
        ERROR(AE_SCREEN_OPEN);
        return;
    }

    g_scrlist[id-1] = scr;
    g_active_scr    = scr;
    g_active_scr_id = id;
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
void WINDOW(SHORT id, UBYTE *title, BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, SHORT flags, SHORT scrid)
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

        if (!g_active_scr)
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
            w = g_active_scr->Width;
            h = g_active_scr->Height;
        }
    }

    g_nw.LeftEdge   = x1;
    g_nw.TopEdge    = y1;
    g_nw.Width      = w;
    g_nw.Height     = h;
    g_nw.Title      = title ? (UBYTE *) _astr_dup(title) : (UBYTE*) "";

    g_nw.Flags      = GIMMEZEROZERO | ACTIVATE;
    g_nw.IDCMPFlags = RAWKEY | ACTIVEWINDOW; // INTUITICKS | VANILLAKEY | MENUPICK | GADGETUP | ACTIVEWINDOW;

    if (flags & AW_FLAG_SIZE)       { g_nw.Flags |= WINDOWSIZING; g_nw.IDCMPFlags |= NEWSIZE;       }
    if (flags & AW_FLAG_DRAG)       { g_nw.Flags |= WINDOWDRAG  ; g_nw.IDCMPFlags |= REFRESHWINDOW; }
    if (flags & AW_FLAG_DEPTH)      { g_nw.Flags |= WINDOWDEPTH ; g_nw.IDCMPFlags |= REFRESHWINDOW; }
    if (flags & AW_FLAG_CLOSE)      { g_nw.Flags |= WINDOWCLOSE ; g_nw.IDCMPFlags |= CLOSEWINDOW;   }
    if (flags & AW_FLAG_BACKDROP)   { g_nw.Flags |= BACKDROP    ;                                   }
    if (flags & AW_FLAG_BORDERLESS) { g_nw.Flags |= BORDERLESS  ;                                   }

    if (g_active_scr)
    {
        g_nw.Screen   = g_active_scr;
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
    g_rp            = win->RPort;

    Move(g_rp, 0, g_rp->Font->tf_YSize - 2);
    SetAPen(g_rp, 1L);

    _g_signalmask_awindow |= (1L << win->UserPort->mp_SigBit);

    g_output_win    = win;
    g_output_win_id = id;

    if (id == 1)
        g_win1_is_dos = FALSE;
}

/*
 * WINDOW CLOSE id
 */
void WINDOW_CLOSE(short id)
{
    // error checking
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (g_winlist[id-1] == NULL) )
    {
        ERROR(AE_WIN_CLOSE);
        return;
    }
    CloseWindow(g_winlist[id-1]);
    g_winlist[id-1]=NULL;
}

/*
 * WINDOW OUTPUT id
 */
void WINDOW_OUTPUT(short id)
{
    // error checking
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (g_winlist[id-1] == NULL) )
    {
        ERROR(AE_WIN_OUTPUT);
        return;
    }

    g_output_win_id = id;

    if ((id != 1) || !g_win1_is_dos)
    {
        struct Window *win = g_winlist[id-1];

        g_output_win    = win;
        g_rp            = win->RPort;
    }
}

void _awindow_shutdown(void)
{
#ifdef ENABLE_DEBUG
    _debug_puts((STRPTR)"_awindow_shutdown ...\n");
#endif
    for (int i = 0; i<MAX_NUM_WINDOWS; i++)
    {
        if (g_winlist[i])
        {
            if (g_winlist[i]->RPort->TmpRas)
            {
#ifdef ENABLE_DEBUG
                _debug_puts((STRPTR)"_awindow_shutdown ... FreeVec RasPtr="); _debug_putu4((ULONG)g_winlist[i]->RPort->TmpRas->RasPtr); _debug_putnl();
                Delay (100);
#endif
                FreeVec ((PLANEPTR) g_winlist[i]->RPort->TmpRas->RasPtr);
#ifdef ENABLE_DEBUG
                _debug_puts((STRPTR)"_awindow_shutdown ... FreeVec TmpRas="); _debug_putu4((ULONG)g_winlist[i]->RPort->TmpRas); _debug_putnl();
                Delay (100);
#endif
                FreeVec (g_winlist[i]->RPort->TmpRas);
            }
#ifdef ENABLE_DEBUG
            _debug_puts((STRPTR)"_awindow_shutdown ... closing window\n");
            Delay (100);
#endif
            CloseWindow(g_winlist[i]);
        }
    }
    for (int i = 0; i<MAX_NUM_SCREENS; i++)
    {
        if (g_scrlist[i])
        {
#ifdef ENABLE_DEBUG
            _debug_puts((STRPTR)"_awindow_shutdown ... CloseScreen\n");
            Delay (100);
#endif
            CloseScreen(g_scrlist[i]);
        }
    }
    _aio_set_dos_cursor_visible (TRUE);
    if (g_console_device_opened)
    {
#ifdef ENABLE_DEBUG
        _debug_puts((STRPTR)"_awindow_shutdown ... CloseDevice\n");
        Delay (100);
#endif
        CloseDevice((struct IORequest *)&g_ioreq);
    }
#ifdef ENABLE_DEBUG
    _debug_puts((STRPTR)"_awindow_shutdown ... finished\n");
    Delay (100);
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
    g_win1_is_dos = _startup_mode == STARTUP_CLI;
}

static BOOL _checkCurWinDos (void)
{
    if (g_output_win_id != 1)
        return FALSE;

    if (g_win1_is_dos)
        return TRUE;

    // auto-open window ?

    if (!g_winlist[0])
        WINDOW (/*id=*/1, /*title=*/(STRPTR)"AQB Output", /*s1=*/FALSE, /*x1=*/-1, /*y1=*/-1, /*s2=*/FALSE, /*x2=*/-1, /*y2=*/-1, /*flags=*/15, /*scrid=*/0);

    return FALSE;
}

/*
 * CLS
 */

void CLS (void)
{
    CHKBRK;
    if (_checkCurWinDos())
    {
        char form_feed = 0x0c;
        Write(g_stdout, (CONST APTR) &form_feed, 1);
        return;
    }

    Move (g_rp, 0, 0);
    ClearScreen(g_rp);
    LOCATE(1,1);
}

/*
 * LINE [ [ STEP ] ( x1 , y1 ) ] - [ STEP ] ( x2 , y2 ) [, [ Color ]  [, flag ] ]
 */
void LINE(BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short c, short bf)
{
    CHKBRK;
    BYTE fgPen=g_rp->FgPen;
#if 0
    _debug_puts((STRPTR)"s1: ")  ; _debug_puts2(s1);
    _debug_puts((STRPTR)", x1: "); _debug_puts2(x1);
    _debug_puts((STRPTR)", y1: "); _debug_puts2(y1);
    _debug_puts((STRPTR)", s2: "); _debug_puts2(s2);
    _debug_puts((STRPTR)", x2: "); _debug_puts2(x2);
    _debug_puts((STRPTR)", y2: "); _debug_puts2(y2);
    _debug_puts((STRPTR)", c: ") ; _debug_puts2(c);
    _debug_puts((STRPTR)", bf: "); _debug_puts2(bf);
    _debug_putnl();
#endif

    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp )
    {
        ERROR(AE_LINE);
        return;
    }

    if (x1<0)
        x1 = g_rp->cp_x;
    if (y1<0)
        y1 = g_rp->cp_y;

    if (s1)
    {
        x1 += g_rp->cp_x;
        y1 += g_rp->cp_y;
    }
    if (s2)
    {
        x2 += g_rp->cp_x;
        y2 += g_rp->cp_y;
    }
    if (c >= 0)
        SetAPen(g_rp, c);
    if (bf & 1)
    {
        if (bf & 2)
        {
            RectFill (g_rp, x1, y1, x2, y2);
        }
        else
        {
            Move (g_rp, x1, y1);
            Draw (g_rp, x2, y1);
            Draw (g_rp, x2, y2);
            Draw (g_rp, x1, y2);
            Draw (g_rp, x1, y1);
        }
    }
    else
    {
        Move (g_rp, x1, y1);
        Draw (g_rp, x2, y2);
    }

    if ( c >=0 )
        SetAPen(g_rp, fgPen);
}

void PSET(BOOL s, short x, short y, short color)
{
    CHKBRK;
    BYTE fgPen=g_rp->FgPen;

    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp )
    {
        ERROR(AE_PSET);
        return;
    }

    if (s)
    {
        x += g_rp->cp_x;
        y += g_rp->cp_y;
    }
    // if (flags & AW_PSET_RESET)
    //    SetAPen(g_rp, g_rp->BgPen);
    if (color >= 0)
        SetAPen(g_rp, color);

    Move (g_rp, x, y);
    WritePixel(g_rp, x, y);

    //if ( (flags & AW_PSET_RESET) || color >=0 )
    //    SetAPen(g_rp, fgPen);
    if ( color >=0 )
        SetAPen(g_rp, fgPen);
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

void SLEEP(void)
{
    CHKBRK;

    struct IntuiMessage *message = NULL;

    ULONG signals = Wait (_g_signalmask_awindow | _g_signalmask_atimer);
#ifdef ENABLE_DEBUG
    _debug_puts((STRPTR)"sleep: got one ore more signals: "); _debug_putu4(signals); _debug_putnl();
#endif

    if (signals & _g_signalmask_atimer)
        _atimer_process_signals(signals);

    for (int i =0; i<MAX_NUM_WINDOWS; i++)
    {
        // _debug_puts((STRPTR)"sleep: checking win "); _debug_puts4(i); _debug_putnl();
        struct Window *win = g_winlist[i];
        if (!win)
            continue;

        if (!(signals & (1L << win->UserPort->mp_SigBit)) )
            continue;

        while ( (message = (struct IntuiMessage *)GetMsg(win->UserPort) ) )
        {
            ULONG class = message->Class;

#ifdef ENABLE_DEBUG
            //_debug_puts((STRPTR)"sleep: got a message, class="); _debug_puts4(class); _debug_putnl();
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
                    //_debug_puts((STRPTR)"SLEEP: MOUSEMOVE\n");
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
            if (!g_output_win)
                return 0;
            return g_output_win->GZZWidth;
        case 3:                                 //  3: current output window height
            if (!g_output_win)
                return 0;
            return g_output_win->GZZHeight;
        case 4:                                 //  4: current output cursor X
            if (!g_rp)
                return 0;
            return g_rp->cp_x;
        case 5:                                 //  5: current output cursor Y
            if (!g_rp)
                return 0;
            return g_rp->cp_y;
        case 6:                                 //  6: highest color index
            if (!g_rp)
                return 0;
            return (1<<g_rp->BitMap->Depth)-1;
        case 7:                                 //  7: pointer to current intuition output window
            return (ULONG) g_output_win;
        case 8:                                 //  8: pointer to current rastport
            return (ULONG) g_rp;
        case 9:                                 //  9: output file handle (ACE)
            return (ULONG) g_stdout;
        case 10:                                // 10: foreground pen (ACE)
            if (!g_rp)
                return 0;
            return g_rp->FgPen;
        case 11:                                // 11: background pen (ACE)
            if (!g_rp)
                return 0;
            return g_rp->BgPen;
        case 12:                                // 12: text width (ACE)
            if (!g_rp)
                return 0;
            return g_rp->TxWidth;
        case 13:                                // 13: text height (ACE)
            if (!g_rp)
                return 0;
            return g_rp->TxHeight;
        case 14:                                // 14: input file handle (AQB)
            return (ULONG) g_stdin;

    }
    return 0;
}

void MOUSE_ON (void)
{
    if ( ((g_output_win_id == 1) && g_win1_is_dos) || !g_output_win )
    {
        ERROR(AE_MOUSE);
        return;
    }

    ModifyIDCMP (g_output_win, g_output_win->IDCMPFlags | MOUSEBUTTONS);
}

void MOUSE_OFF (void)
{
    if ( ((g_output_win_id == 1) && g_win1_is_dos) || !g_output_win )
    {
        ERROR(AE_MOUSE);
        return;
    }

    ModifyIDCMP (g_output_win, g_output_win->IDCMPFlags & ~MOUSEBUTTONS);
}

void ON_MOUSE_CALL (void (*cb)(void))
{
    g_mouse_cb = cb;
}

WORD MOUSE_ (SHORT n)
{
    if ( ((g_output_win_id == 1) && g_win1_is_dos) || !g_output_win )
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
            return g_output_win->GZZMouseX;
        case 2:
            return g_output_win->GZZMouseY;

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
    if ( ((g_output_win_id == 1) && g_win1_is_dos) || !g_output_win )
    {
        ERROR(AE_MOUSE);
        return;
    }

    g_output_win->Flags |= REPORTMOUSE;
    ModifyIDCMP (g_output_win, g_output_win->IDCMPFlags | MOUSEMOVE);
    //_debug_puts((STRPTR)"MOUSE_MOTION_ON\n");
}

void MOUSE_MOTION_OFF (void)
{
    if ( ((g_output_win_id == 1) && g_win1_is_dos) || !g_output_win )
    {
        ERROR(AE_MOUSE);
        return;
    }

    g_output_win->Flags &= ~REPORTMOUSE;
    ModifyIDCMP (g_output_win, g_output_win->IDCMPFlags & ~MOUSEMOVE);
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
    WORD max_lines = g_output_win->GZZHeight / g_rp->Font->tf_YSize - 1;
    WORD cy = g_rp->cp_y / g_rp->Font->tf_YSize;
    WORD scroll_y = cy - max_lines;

    if (scroll_y > 0)
    {
        scroll_y *= g_rp->Font->tf_YSize;
        ScrollRaster (g_rp, 0, scroll_y, 0, 0, g_output_win->GZZWidth, g_output_win->GZZHeight);
        Move (g_rp, g_rp->cp_x, g_rp->cp_y-scroll_y);
    }
}

void _aio_puts(USHORT fno, const UBYTE *s)
{
    CHKBRK;

    //_debug_puts((STRPTR)"_debug_puts\n");

    if (fno)
    {
        _aio_fputs (fno, s);
        return;
    }

    if (_checkCurWinDos())
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
                    Text (g_rp, (UBYTE *) &s[startpos], length);

                switch (ch)
                {
                    case 0:         // string end marker
                        goto fini;

                    case 7:         // bell
                        DisplayBeep(NULL);
                        break;

                    case 8:         // backspace
                        if (g_rp->cp_x >= g_rp->Font->tf_XSize)
                        {
                            Move (g_rp, g_rp->cp_x-g_rp->Font->tf_XSize, g_rp->cp_y);
                            Text (g_rp, (UBYTE*) " ", 1);
                            Move (g_rp, g_rp->cp_x-g_rp->Font->tf_XSize, g_rp->cp_y);
                        }
                        break;

                    case 9:         // tab
                    {
                        int cx = g_rp->cp_x / g_rp->Font->tf_XSize;          // cursor position in nominal characters
                        // _debug_puts((STRPTR)"[1] cx="); _debug_puts2(cx); _debug_puts((STRPTR)"\n");
                        cx = cx + (8-(cx%8));                                // AmigaBASIC TABs are 9 characters wide
                        // _debug_puts((STRPTR)"[2] cx="); _debug_puts2(cx); _debug_puts((STRPTR)"\n");
                        Move (g_rp, cx * g_rp->Font->tf_XSize, g_rp->cp_y);
                        break;
                    }
                    case 10:        // linefeed
                        Move (g_rp, 0, g_rp->cp_y + g_rp->Font->tf_YSize);
                        do_scroll();
                        break;

                    case 12:        // clear screen
                        Move (g_rp, 0, 0);
                        SetRast (g_rp, g_rp->BgPen);
                        Move (g_rp, 0, g_rp->Font->tf_Baseline);
                        break;

                    case 13:        // carriage return
                        Move (g_rp, 0, g_rp->cp_y);
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

    if (_checkCurWinDos())
    {
        Write(g_stdout, (CONST APTR) "\t", 1);
        return;
    }

    int cx = g_rp->cp_x / g_rp->Font->tf_XSize;          // cursor position in nominal characters
    cx = cx + (14-(cx%14));                              // PRINT comma TABs are 15 characters wide
    Move (g_rp, cx * g_rp->Font->tf_XSize, g_rp->cp_y);
}

#define CSI 0x9b

void LOCATE (SHORT l, SHORT c)
{
    if (_checkCurWinDos())
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

    Move (g_rp, c * g_rp->Font->tf_XSize, l * g_rp->Font->tf_YSize + g_rp->Font->tf_Baseline);
}

SHORT CSRLIN_ (void)
{
    if ( (g_output_win_id == 1) && g_win1_is_dos )
        return 0;

    return g_rp->cp_y / g_rp->Font->tf_YSize + 1;
}

SHORT POS_ (SHORT dummy)
{
    if ( (g_output_win_id == 1) && g_win1_is_dos )
        return 0;

    return g_rp->cp_x / g_rp->Font->tf_XSize + 1;
}

void SLEEP_FOR (FLOAT s)
{
    LONG ticks = SPFix(SPMul(s, g_fp50));

    if (ticks <= 0)
        return;

    while (ticks > 25)
    {
        CHKBRK;
        Delay (25);
        ticks -= 25;
    }

    CHKBRK;
    Delay (ticks);
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

    old_fg = g_rp->FgPen;
    old_x  = g_rp->cp_x;
    old_y  = g_rp->cp_y;

    SetAPen (g_rp, (1<<g_rp->BitMap->Depth)-1);
    RectFill (g_rp, old_x+1, old_y - g_rp->TxBaseline,
                    old_x+2, old_y - g_rp->TxBaseline + g_rp->TxHeight - 1);

    Move (g_rp, old_x, old_y);
    SetAPen (g_rp, old_fg);
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

    if (_checkCurWinDos())
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
            char *buf2 = INKEY_();
            char c = *buf2;

            if (c == '\0')
                continue;

            if (c != '\b')
            {
                if (is_eol(c))
                    break;

                Text (g_rp, (UBYTE *) buf2, 1);
                draw_cursor();

                buf[col] = c;
                col++;
            }
            else
            {
                if (col > 0)
                {
                    int x = g_rp->cp_x - g_rp->Font->tf_XSize;
                    int y = g_rp->cp_y;

                    // erase last char + cursor
                    Move(g_rp, x, y);
                    Text(g_rp, (UBYTE*) &twospaces, 2);

                    // draw new cursor
                    Move(g_rp, x, y);
                    draw_cursor();

                    col--;
                }
            }
        }

        buf[col] = '\0';

        // cleanup cursor
        Text (g_rp, (UBYTE *) &twospaces, 1);

        if (do_nl)
        {
            //_debug_puts((STRPTR)"do_nl");
            Move (g_rp, 0, g_rp->cp_y + g_rp->Font->tf_YSize);
            do_scroll();
        }
        else
        {
            //_debug_puts((STRPTR)"NOT do_nl");
        }
    }

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
    if (!g_active_scr)
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

    SetRGB4(&g_active_scr->ViewPort, cid, r, g, b);
}

void COLOR(short fg, short bg, short o)
{
    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp )
    {
        ERROR(AE_COLOR);
        return;
    }

    if (fg >= 0)
        SetAPen (g_rp, fg);
    if (bg >= 0)
        SetBPen (g_rp, bg);
    if (o >= 0)
        g_rp->AOlPen = o;
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
    ULONG rassize = RASSIZE (g_output_win->Width, g_output_win->Height);

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
    g_rp->TmpRas = aTmpRas;
}

void PAINT(BOOL s, short x, short y, short pc, short aol)
{
    CHKBRK;

    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp )
    {
        ERROR(AE_PAINT);
        return;
    }

    // init tmp raster if not done yet
    if (!g_rp->TmpRas)
        allocTmpRas();

    if (s)
    {
        x += g_rp->cp_x;
        y += g_rp->cp_y;
    }

    BYTE fgPen=g_rp->FgPen;
    BYTE aolPen=g_rp->AOlPen;
    if (pc >= 0)
        SetAPen(g_rp, pc);
    if (aol >= 0)
        g_rp->AOlPen = aol;

    Flood (g_rp, 0, x, y);

    if ( pc >=0 )
        SetAPen(g_rp, fgPen);
    if ( aol >= 0 )
        g_rp->AOlPen = aolPen;
}

#define AREA_MAX_CNT    100

void AREA(BOOL s, short x, short y)
{
    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp )
    {
        ERROR(AE_AREA);
        return;
    }

    // _debug_puts((STRPTR)"AREA: s="); _debug_puts2(s);
    // _debug_puts((STRPTR)", x="); _debug_puts2(x);
    // _debug_puts((STRPTR)", y="); _debug_puts2(y);
    // _debug_putnl();

    // first call on this rp? -> initialize Area* related data structures

    if (!g_rp->AreaInfo)
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
        g_rp->AreaInfo = ai;
    }

    // init tmp raster if not done yet
    if (!g_rp->TmpRas)
        allocTmpRas();

    if (s)
    {
        x += g_rp->cp_x;
        y += g_rp->cp_y;
    }

    WORD cnt = g_rp->AreaInfo->Count;

    if (cnt >= AREA_MAX_CNT)
    {
        ERROR(AE_AREA);
        return;
    }

    if (cnt==0)
        AreaMove (g_rp, x, y);
    else
        AreaDraw (g_rp, x, y);
}

void AREAFILL (short mode)
{
    CHKBRK;

    BYTE dm;

    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp || !g_rp->AreaInfo )
    {
        ERROR(AE_AREA);
        return;
    }

    dm = g_rp->DrawMode;
    if (mode==1)
    {
        SetDrMd(g_rp, COMPLEMENT);
    }
    else
    {
        SetDrMd(g_rp, JAM2);
    }

    AreaEnd(g_rp);

    SetDrMd(g_rp, dm);
}

void AREA_OUTLINE(BOOL enabled)
{
    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp )
    {
        ERROR(AE_AREA);
        return;
    }

    if (enabled)
        g_rp->Flags |= AREAOUTLINE;
    else
        g_rp->Flags &= ~AREAOUTLINE;
}

void PATTERN (unsigned short lineptrn, _DARRAY_T *areaptrn)
{
    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp )
    {
        ERROR(AE_PATTERN);
        return;
    }

    g_rp->LinePtrn   = lineptrn;
    g_rp->Flags     |= FRST_DOT;
    g_rp->linpatcnt  = 15;

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

        g_rp->AreaPtrn = areaptrn->data;
        g_rp->AreaPtSz = ptSz;
    }
}

void PATTERN_RESTORE (void)
{
    if ( ( (g_output_win_id == 1) && g_win1_is_dos) || !g_rp )
    {
        ERROR(AE_PATTERN);
        return;
    }

    g_rp->LinePtrn   = 0xFFFF;
    g_rp->Flags     |= FRST_DOT;
    g_rp->linpatcnt  = 15;
    g_rp->AreaPtrn   = NULL;
    g_rp->AreaPtSz   = 0;
}

#if 0
// FIXME
struct BitMap *BITMAP_ (SHORT width, SHORT height, SHORT depth)
{
    myBitMaps[1] = (struct BitMap *)AllocMem((LONG)sizeof(struct BitMap), MEMF_CLEAR);
    if (myBitMaps[1] != NULL)
    {
        InitBitMap(myBitMaps[0], depth, width, height);
    }
}
#endif

static char inkeybuf[2] = { 0, 0 } ;

char *INKEY_ (void)
{
    CHKBRK;

    if (_checkCurWinDos())
    {
        LONG l = Read(g_stdin, (CONST APTR) inkeybuf, 1);
        if (l != 1)
            return "";
        return inkeybuf;
    }

    if (!keybuf_len)
        SLEEP();

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

