#include "awindow.h"
#include "autil.h"
#include "astr.h"
#include "aio.h"

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <exec/memory.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>

static struct NewWindow g_nw =
{
    0, 0, 0, 0,                                                    // LeftEdge, TopEdge, Width, Height
    0, 1,                                                          // DetailPen, BlockPen
    INTUITICKS | VANILLAKEY | MENUPICK | GADGETUP | ACTIVEWINDOW,  // IDCMPFlags 
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
 * keep track of window ids
 */

#define MAX_NUM_WINDOWS 16

static struct Window * g_winlist[MAX_NUM_WINDOWS] = {
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL
};


static ULONG g_signalmask=0;

static void (*g_win_cb)(void) = NULL;

static short            g_active_win_id = 0;
static short            g_output_win_id = 0;
static struct Window   *g_output_win    = NULL;
static struct RastPort *g_rp            = NULL;

/*
 * BASIC:
 *
 * WINDOW id [, [Title] [, [(x1,y1)-(x2,y2)] [, [Flags] [, Screen] ] ]
 */
BOOL __aqb_window_open(short id, char *title, short x1, short y1, short x2, short y2, short flags, short scr_id)
{
    USHORT w, h;

    // error checking
    if ( (id < 1) || (id > MAX_NUM_WINDOWS) || (g_winlist[id-1] != NULL) || (x1 > x2) || (y1 > y2) )
    {
        g_errcode = AE_WIN_OPEN;
        return FALSE;
    }

    if (x1>=0)
    {
        w  = x2 - x1;
        h  = y2 - y1;
    }
    else
    {
        struct Screen sc;

        // get workbench screen size, calculate inner size for a fullscreen window
        if (!GetScreenData ((APTR) &sc, sizeof(struct Screen), WBENCHSCREEN, NULL))
        {
            g_errcode = AE_WIN_OPEN;
            return FALSE;
        }

        x1 = 0;
        y1 = 0;

        // w = sc.Width  - sc.WBorLeft - sc.WBorRight;
        // h = sc.Height - sc.WBorTop  - sc.WBorBottom;
        w = sc.Width;
        h = sc.Height;
    }

    g_nw.LeftEdge   = x1;
    g_nw.TopEdge    = y1;
    g_nw.Width      = w;
    g_nw.Height     = h;
    g_nw.Title      = title ? (UBYTE *) _astr_dup(title) : NULL;

    g_nw.Flags      = GIMMEZEROZERO | ACTIVATE;
    g_nw.IDCMPFlags = VANILLAKEY | ACTIVEWINDOW; // INTUITICKS | VANILLAKEY | MENUPICK | GADGETUP | ACTIVEWINDOW;

    if (flags & AW_FLAG_SIZE)  { g_nw.Flags |= WINDOWSIZING; g_nw.IDCMPFlags |= NEWSIZE;       }
    if (flags & AW_FLAG_DRAG)  { g_nw.Flags |= WINDOWDRAG  ; g_nw.IDCMPFlags |= REFRESHWINDOW; }
    if (flags & AW_FLAG_DEPTH) { g_nw.Flags |= WINDOWDEPTH ; g_nw.IDCMPFlags |= REFRESHWINDOW; }
    if (flags & AW_FLAG_CLOSE) { g_nw.Flags |= WINDOWCLOSE ; g_nw.IDCMPFlags |= CLOSEWINDOW;   }

    // FIXME: screen
    g_nw.Type     = WBENCHSCREEN;

    struct Window *win = (struct Window *)OpenWindow(&g_nw);

    if (!win)
    {
        g_errcode = AE_WIN_OPEN;
        return FALSE;
    }

    g_winlist[id-1] = win;
    g_rp            = win->RPort;

    Move(g_rp, 0, g_rp->Font->tf_YSize - 2);
    SetAPen(g_rp, 1L);

    g_signalmask |= (1L << win->UserPort->mp_SigBit);

    g_output_win    = win;
    g_output_win_id = id;

    return TRUE;
}

void _awindow_init(void)
{
    // get workbench screen size info
}

void _awindow_shutdown(void)
{
    //_aio_puts("_awindow_shutdown ...\n");
    for (int i = 0; i<MAX_NUM_WINDOWS; i++)
    {
        if (g_winlist[i])
            CloseWindow(g_winlist[i]);
    }
    //_aio_puts("_awindow_shutdown ... done.\n");
}

/*
 * BASIC:
 *
 * LINE [ [ STEP ] ( x1 , y1 ) ] - [ STEP ] ( x2 , y2 ) [, [ Color ]  [, flag ] ]
 */
BOOL __aqb_line(short x1, short y1, short x2, short y2, short flags, short color)
{
    BYTE fgPen=g_rp->FgPen;
#if 0
    _aio_puts("x1: "); _aio_puts4(x1);
    _aio_puts(", y1: "); _aio_puts4(y1);
    _aio_puts(", x2: "); _aio_puts4(x2);
    _aio_puts(", y2: "); _aio_puts4(y2);
    _aio_puts(", flags: "); _aio_puts4(flags);
    _aio_puts(", color: "); _aio_puts4(color);
    _aio_putnl();
#endif

    if (flags & AW_LINE_STEP_1)
    {
        x1 += g_rp->cp_x;
        y1 += g_rp->cp_y;
    }
    if (flags & AW_LINE_STEP_2)
    {
        x2 += g_rp->cp_x;
        y2 += g_rp->cp_y;
    }
    if (color >= 0)
        SetAPen(g_rp, color);
    if (flags & AW_LINE_FLAG_BOX)
    {
        // FIXME
        if (flags & AW_LINE_FLAG_FILL)
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

    if ( color >=0 )
        SetAPen(g_rp, fgPen);

    return TRUE;
}

/*
 * BASIC:
 *
 * (PSET|PRESET) [ STEP ] ( x , y ) [ , Color ]
 */
BOOL __aqb_pset(short x, short y, short flags, short color)
{
    BYTE fgPen=g_rp->FgPen;

    if (flags & AW_PSET_STEP)
    {
        x += g_rp->cp_x;
        y += g_rp->cp_y;
    }
    if (flags & AW_PSET_RESET)
        SetAPen(g_rp, g_rp->BgPen);
    if (color >= 0)
        SetAPen(g_rp, color);

    Move (g_rp, x, y);
    WritePixel(g_rp, x, y);

    if ( (flags & AW_PSET_RESET) || color >=0 )
        SetAPen(g_rp, fgPen);

    return TRUE;
}

/* BASIC: SLEEP
   event handling */

void __aqb_sleep(void)
{
    struct IntuiMessage *message = NULL;

    ULONG signals = Wait (g_signalmask);
    _aio_puts("sleep: got a signal.\n");

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

            _aio_puts("sleep: got a message, class="); _aio_puts4(class); _aio_putnl();

            switch(class)
            {
                case CLOSEWINDOW:
                    if (g_win_cb)
                        g_win_cb();
                    break;
                case ACTIVEWINDOW:
                    g_active_win_id = i+1;
                    break;
            }

            ReplyMsg ( (struct Message *) message);
        }
    }
}

void __aqb_on_window_call(void (*cb)(void))
{
    g_win_cb = cb;
}

ULONG __aqb_window_fn(short n)
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

    }
    return 0;
}
