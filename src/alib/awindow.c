#include "awindow.h"
#include "autil.h"
#include "astr.h"
#include "aio.h"

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <exec/memory.h>
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

static struct RastPort *g_rp=NULL;

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

    g_nw.LeftEdge = x1;
    g_nw.TopEdge  = y1;
    g_nw.Width    = w;
    g_nw.Height   = h;
    g_nw.Title    = title ? (UBYTE *) _astr_dup(title) : NULL;

    g_nw.Flags    = GIMMEZEROZERO | ACTIVATE;     
    
    // FIXME: IDCMPFlags
    if (flags & AW_FLAG_SIZE)  g_nw.Flags |= WINDOWSIZING;
    if (flags & AW_FLAG_DRAG)  g_nw.Flags |= WINDOWDRAG;
    if (flags & AW_FLAG_DEPTH) g_nw.Flags |= WINDOWDEPTH;
    if (flags & AW_FLAG_CLOSE) g_nw.Flags |= WINDOWCLOSE;

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
#if 0
    Move (g_rp, 10, 10);
    Draw (g_rp, 15, 15);
#endif
    return TRUE;
}

