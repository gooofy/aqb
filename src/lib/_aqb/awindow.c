#include "_aqb.h"

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

BPTR g_stdout, g_stdin;

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

static short            g_active_win_id = 1;
static short            g_output_win_id = 1;
static struct Window   *g_output_win    = NULL;
static struct RastPort *g_rp            = NULL;
static BOOL             g_win1_is_dos   = TRUE; // window 1 is the DOS stdout unless re-opened

/*
 * WINDOW id [, [Title] [, [(x1,y1)-(x2,y2)] [, [Flags] [, Screen] ] ]
 */
BOOL WINDOW(short id, char *title, BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short flags, short scrid)
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

    if (id == 1)
        g_win1_is_dos = FALSE;

    return TRUE;
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

void _awindow_init(void)
{
    g_stdout = Output();
    g_stdin  = Input();
}

/*
 * LINE [ [ STEP ] ( x1 , y1 ) ] - [ STEP ] ( x2 , y2 ) [, [ Color ]  [, flag ] ]
 */
BOOL LINE(BOOL s1, short x1, short y1, BOOL s2, short x2, short y2, short c, short bf)
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

    return TRUE;
}

void PSET(BOOL s, short x, short y, short color)
{
    BYTE fgPen=g_rp->FgPen;

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

#define MAXKEYBUF 256

static char keybuf[MAXKEYBUF];
static int  keybuf_start = 0;
static int  keybuf_end   = 0;

void SLEEP(void)
{
    struct IntuiMessage *message = NULL;

    ULONG signals = Wait (g_signalmask);
    // _aio_puts("sleep: got a signal.\n");

    for (int i =0; i<MAX_NUM_WINDOWS; i++)
    {
        // _aio_puts("sleep: checking win "); _aio_puts4(i); _aio_putnl();
        struct Window *win = g_winlist[i];
        if (!win)
            continue;

        if (!(signals & (1L << win->UserPort->mp_SigBit)) )
            continue;

        while ( (message = (struct IntuiMessage *)GetMsg(win->UserPort) ) )
        {
            ULONG class = message->Class;

            // _aio_puts("sleep: got a message, class="); _aio_puts4(class); _aio_putnl();

            switch(class)
            {
                case CLOSEWINDOW:
                    // _aio_puts("sleep: CLOSEWINDOW"); _aio_putnl();
                    if (g_win_cb)
                    {
                        // _aio_puts("sleep: callback."); _aio_putnl();
                        g_win_cb();
                    }
                    break;
                case ACTIVEWINDOW:
                    g_active_win_id = i+1;
                    break;

                case VANILLAKEY:
                    keybuf[keybuf_end] = message->Code;
                    keybuf_end = (keybuf_end + 1) % MAXKEYBUF;
                    break;
            }

            ReplyMsg ( (struct Message *) message);
            //_aio_puts("sleep: replied.\n");
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

    }
    return 0;
}

/*
 * print statement support
 */

static void do_scroll(void)
{
    WORD max_lines = g_output_win->GZZHeight / g_rp->Font->tf_YSize;
    WORD cy = g_rp->cp_y / g_rp->Font->tf_YSize;
    WORD scroll_y = cy - max_lines;

    if (scroll_y > 0)
    {
        scroll_y *= g_rp->Font->tf_YSize;
        ScrollRaster (g_rp, 0, scroll_y, 0, 0, g_output_win->GZZWidth, g_output_win->GZZHeight);
        Move (g_rp, g_rp->cp_x, g_rp->cp_y-scroll_y);
    }
}

void _aio_puts(const char *s)
{
    if ( (g_output_win_id == 1) && g_win1_is_dos)
    {
        Write(g_stdout, (CONST APTR) s, len_(s));
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
                        // _debug_puts("[1] cx="); _debug_puts2(cx); _debug_puts("\n");
                        cx = cx + (8-(cx%8));                                // AmigaBASIC TABs are 9 characters wide
                        // _debug_puts("[2] cx="); _debug_puts2(cx); _debug_puts("\n");
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

void _aio_puttab(void)
{
    if ( (g_output_win_id == 1) && g_win1_is_dos)
    {
        Write(g_stdout, (CONST APTR) "\t", 1);
        return;
    }

    int cx = g_rp->cp_x / g_rp->Font->tf_XSize;          // cursor position in nominal characters
    cx = cx + (14-(cx%14));                              // PRINT comma TABs are 15 characters wide
    Move (g_rp, cx * g_rp->Font->tf_XSize, g_rp->cp_y);
}

void LOCATE (short l, short c)
{
    if (l<0)
        l = CSRLIN_();
    if (c<0)
        c = POS_(0);

    Move (g_rp, c * g_rp->Font->tf_XSize, l * g_rp->Font->tf_YSize + g_rp->Font->tf_Baseline);
}

short CSRLIN_ (void)
{
    if ( (g_output_win_id == 1) && g_win1_is_dos )
        return 0;

    return g_rp->cp_y / g_rp->Font->tf_YSize;
}

short POS_ (short dummy)
{
    if ( (g_output_win_id == 1) && g_win1_is_dos )
        return 0;

    return g_rp->cp_x / g_rp->Font->tf_XSize;
}

static char inkeybuf[2] = { 0, 0 } ;

char *INKEY_ (void)
{
    if ( (g_output_win_id == 1) && g_win1_is_dos)
    {
        LONG l = Read(g_stdin, (CONST APTR) inkeybuf, 1);
        if (l != 1)
            return "";
        return inkeybuf;
    }

    if (keybuf_start == keybuf_end)
    {
        SLEEP();
    }

    if (keybuf_start == keybuf_end)
    {
        inkeybuf[0] = 0;
        return inkeybuf;
    }

    inkeybuf[0] = keybuf[keybuf_start];
    keybuf_start++;

    return inkeybuf;
}

