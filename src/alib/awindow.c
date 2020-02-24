#include "awindow.h"
#include "autil.h"
#include "astr.h"
#include "aio.h"

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <exec/memory.h>

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

    // FIXME: set first drawing position
    // Move(RPort,0,RPort->Font->tf_YSize - 2);
    // SetAPen(RPort,1L);

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


#if 0

    *((ULONG *)(&scbox.Left)) = (ULONG) 0;
    *((ULONG *)(&scbox.Width)) = *((ULONG *)(&sc->Width));

    /*
     * new "inner" dimensions processing
     * (you probably want AUTOADJUST).
     */
    if ( ( inner = GetUserTagData( WA_InnerWidth, -1, tags ) ) != -1 )
    {
    DOW( printf("doing inner width\n") );
    wbox.Width = inner + window->BorderLeft + window->BorderRight;
    }
    if ( ( inner = GetUserTagData( WA_InnerHeight, -1, tags ) ) != -1 )
    {
    DOW( printf("doing inner height\n") );
    wbox.Height = inner + window->BorderTop + window->BorderBottom;
    }


#endif

