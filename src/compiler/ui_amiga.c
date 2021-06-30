#ifdef __amigaos__

#define INTUI_V36_NAMES_ONLY

#include <stdlib.h>
#include <stdarg.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <devices/conunit.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#include <proto/console.h>
#include <pragmas/console_pragmas.h>

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/console_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/intuition.h>
#include <inline/graphics.h>

#include <libraries/reqtools.h>
#include <inline/reqtools.h>

#include <libraries/gadtools.h>
#include <clib/gadtools_protos.h>
#include <inline/gadtools.h>

#include <clib/console_protos.h>
#include <inline/console.h>

#include "ui.h"
#include "logger.h"
#include "options.h"

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct IntuitionBase *IntuitionBase;
struct Library              *GadToolsBase;
struct ReqToolsBase         *ReqToolsBase;
struct Device               *ConsoleDevice = NULL;

#define NEWLIST(l) ((l)->lh_Head = (struct Node *)&(l)->lh_Tail, \
                    /*(l)->lh_Tail = NULL,*/ \
                    (l)->lh_TailPred = (struct Node *)&(l)->lh_Head)

static struct NewMenu g_newmenu[] =
    {
        { NM_TITLE, (STRPTR) "Project",             0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Open...",   (STRPTR) "O", 0, 0, (APTR)KEY_CTRL_O,},
        {  NM_ITEM, (STRPTR) "Save",      (STRPTR) "S", 0, 0, (APTR)KEY_CTRL_S,},
        {  NM_ITEM, NM_BARLABEL,                    0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Help...",             0 , 0, 0, (APTR)KEY_F1,},
        {  NM_ITEM, (STRPTR) "About...",            0 , 0, 0, (APTR)KEY_ABOUT,},
        {  NM_ITEM, NM_BARLABEL,                    0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Quit...",   (STRPTR) "Q", 0, 0, (APTR)KEY_CTRL_C,},

        { NM_TITLE, (STRPTR) "Edit",                0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Cut",       (STRPTR) "X", 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Copy",      (STRPTR) "C", 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Paste",     (STRPTR) "V", 0, 0, 0,},

        { NM_TITLE, (STRPTR) "Settings",            0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Custom Screen",       0 , CHECKIT | MENUTOGGLE,   0, (APTR)KEY_CUSTOMSCREEN,},
        {  NM_ITEM, (STRPTR) "Colorscheme",         0 , 0, 0, 0,},
        {   NM_SUB, (STRPTR) "Super dark",          0 , CHECKIT | MENUTOGGLE,  ~1, (APTR)KEY_COLORSCHEME_0,},
        {   NM_SUB, (STRPTR) "Dark blue",           0 , CHECKIT | MENUTOGGLE,  ~2, (APTR)KEY_COLORSCHEME_1,},
        {   NM_SUB, (STRPTR) "QB",                  0 , CHECKIT | MENUTOGGLE,  ~4, (APTR)KEY_COLORSCHEME_2,},
        {   NM_SUB, (STRPTR) "TP",                  0 , CHECKIT | MENUTOGGLE,  ~8, (APTR)KEY_COLORSCHEME_3,},
        {   NM_SUB, (STRPTR) "OS 2.0",              0 , CHECKIT | MENUTOGGLE, ~16, (APTR)KEY_COLORSCHEME_4,},
        {   NM_END, NULL, 0 , 0, 0, 0,},
    };


static struct Screen     *g_screen        = NULL;
static struct Window     *g_win           = NULL;
static struct RastPort   *g_rp            = NULL;
static struct FileHandle *g_output        = NULL;
static struct MsgPort    *g_IOport        = NULL;
static struct IOStdReq    console_ioreq;
static UWORD              g_OffLeft, g_OffRight, g_OffTop, g_OffBottom;
static UWORD              g_BMOffTop          = 0;
static int                g_termSignalBit     = -1;
static APTR               g_vi                = NULL;
static struct Menu       *g_menuStrip         = NULL;
static UWORD              g_fontHeight        = 8;
// non-RTG direct-to-bitmap rendering
static struct TextFont   *g_font              = NULL;
static UBYTE              g_fontData[256][8];
static bool               g_renderRTG         = FALSE;
static UWORD              g_renderBMBytesPerRow;
static UBYTE             *g_renderBMPlanes[2];
static UBYTE             *g_renderBMPtr[2];
static bool               g_renderBMPE[2]     = { TRUE, FALSE }; // PE: PlaneEnabled
static bool               g_renderInverse     = FALSE;
static uint16_t           g_renderBMcurCol    = 0;
static uint16_t           g_renderBMcurRow    = 1;
static uint16_t           g_renderBMmaxCols   = 80;
// RTG graphics library based rendering
#define BUFSIZE 1024
static char               g_outbuf[BUFSIZE];
static int                g_bpos = 0;
static UI_size_cb         g_size_cb           = NULL;
static void              *g_size_cb_user_data = NULL;
static UI_key_cb          g_key_cb            = NULL;
static void              *g_key_cb_user_data  = NULL;
static uint16_t           g_scrollStart       = 1;
static uint16_t           g_scrollEnd         = 10;
static bool               g_cursorVisible     = FALSE;
static uint16_t           g_cursorRow         = 1;
static uint16_t           g_cursorCol         = 1;

typedef struct
{
    char   *name;
    UWORD   palette[8];
    WORD    pens3d[10];
} UI_theme_t;

#define NUM_THEMES 5

static UI_theme_t g_themes[NUM_THEMES] = {
    {
        "AQB super dark",
        { 0x0000, 0x0bbb, 0x05af, 0x006a,  },
        // DETAILPEN, BLOCKPEN, TEXTPEN, SHINEPEN, SHADOWPEN, FILLPEN, FILLTEXTPEN, BACKGROUNDPEN, HIGHLIGHTTEXTPEN
        {          0,        1,       1,        2,         3,       0,           0,             0,                2, -1},
    },
    {
        "AQB dark blue",
        { 0x0004, 0x0bbb, 0x05af, 0x006a,  },
        // DETAILPEN, BLOCKPEN, TEXTPEN, SHINEPEN, SHADOWPEN, FILLPEN, FILLTEXTPEN, BACKGROUNDPEN, HIGHLIGHTTEXTPEN
        {          0,        1,       1,        2,         3,       0,           0,             0,                2, -1},
    },
    {
        "QB",
        { 0x000a, 0x0ddd, 0x09ce, 0x059a },
        // DETAILPEN, BLOCKPEN, TEXTPEN, SHINEPEN, SHADOWPEN, FILLPEN, FILLTEXTPEN, BACKGROUNDPEN, HIGHLIGHTTEXTPEN
        {          0,        1,       1,        2,         3,       0,           0,             0,                2, -1},
    },
    {
        "TP",
        { 0x000a, 0x0ee0, 0x09ce, 0x059a },
        // DETAILPEN, BLOCKPEN, TEXTPEN, SHINEPEN, SHADOWPEN, FILLPEN, FILLTEXTPEN, BACKGROUNDPEN, HIGHLIGHTTEXTPEN
        {          0,        1,       1,        2,         3,       0,           0,             0,                2, -1},
    },
    {
        "OS 2.0",
        { 0x0aaa, 0x0000, 0x0fff, 0x047a},
        // DETAILPEN, BLOCKPEN, TEXTPEN, SHINEPEN, SHADOWPEN, FILLPEN, FILLTEXTPEN, BACKGROUNDPEN, HIGHLIGHTTEXTPEN
        {          0,        1,       1,        2,         3,       0,           0,             0,                2, -1},
    },
};

static struct MsgPort *create_port(STRPTR name, LONG pri)
{
    struct MsgPort *port = NULL;
    UBYTE portsig;

    if ((BYTE)(portsig=AllocSignal(-1)) >= 0)
    {
        if (!(port=AllocMem(sizeof(*port),MEMF_CLEAR|MEMF_PUBLIC)))
        {
            FreeSignal(portsig);
        }
        else
        {
            port->mp_Node.ln_Type = NT_MSGPORT;
            port->mp_Node.ln_Pri  = pri;
            port->mp_Node.ln_Name = (char *)name;
            /* done via AllocMem
            port->mp_Flags        = PA_SIGNAL;
            */
            port->mp_SigBit       = portsig;
            port->mp_SigTask      = FindTask(NULL);
            NEWLIST(&port->mp_MsgList);
            if (port->mp_Node.ln_Name)
                AddPort(port);
        }
    }
    return port;
}

static void delete_port(struct MsgPort *port)
{
    if (port->mp_Node.ln_Name)
        RemPort(port);
    FreeSignal(port->mp_SigBit);
    FreeMem(port,sizeof(*port));
}
static void cleanexit (char *s, uint32_t n)
{
    if (s)
    {
        printf("%s\n", s);
        U_request(NULL, NULL, "OK", "%s", s);
    }
    exit(n);
}

static void drawCursor(void)
{
    uint16_t x = (g_cursorCol-1)*8 + g_OffLeft;
    uint16_t y = (g_cursorRow-1)*g_fontHeight + g_OffTop;
    BYTE DrawMode = g_rp->DrawMode;
    SetDrMd (g_rp, COMPLEMENT);
    g_rp->Mask = 3;
    //LOG_printf (LOG_DEBUG, "ui_amiga: drawCursor x=%d, y=%d\n", x, y);
    RectFill (g_rp, x, y, x+7, y+g_fontHeight-1);
    SetDrMd (g_rp, DrawMode);
}

static void UI_flush(void)
{
    if (!g_renderRTG)
        return;
    if (g_cursorVisible)
        drawCursor();
    Text (g_rp, (STRPTR) g_outbuf, g_bpos);
    if (g_cursorVisible)
        drawCursor();
    g_bpos = 0;
}

void UI_setTextStyle (uint16_t style)
{
	if (g_screen)
	{
        if (g_renderRTG)
        {
            UI_flush();
            switch (style)
            {
                case UI_TEXT_STYLE_TEXT     : SetAPen (g_rp, 1); SetBPen (g_rp, 0); SetDrMd (g_rp, JAM2); break;
                case UI_TEXT_STYLE_KEYWORD  : SetAPen (g_rp, 2); SetBPen (g_rp, 0); SetDrMd (g_rp, JAM2); break;
                case UI_TEXT_STYLE_COMMENT  : SetAPen (g_rp, 3); SetBPen (g_rp, 0); SetDrMd (g_rp, JAM2); break;
                case UI_TEXT_STYLE_INVERSE  : SetAPen (g_rp, 0); SetBPen (g_rp, 1); SetDrMd (g_rp, JAM2); break;
                default:
                    printf ("UI style %d is unknown.\n", style);
                    assert(FALSE);
            }
        }
        else
        {
            switch (style)
            {
                case UI_TEXT_STYLE_TEXT:
                    g_renderBMPE[0] = TRUE;
                    g_renderBMPE[1] = FALSE;
                    g_renderInverse = FALSE;
                    break;
                case UI_TEXT_STYLE_KEYWORD:
                    g_renderBMPE[0] = FALSE;
                    g_renderBMPE[1] = TRUE;
                    g_renderInverse = FALSE;
                    break;
                case UI_TEXT_STYLE_COMMENT:
                    g_renderBMPE[0] = TRUE;
                    g_renderBMPE[1] = TRUE;
                    g_renderInverse = FALSE;
                    break;
                case UI_TEXT_STYLE_INVERSE:
                    g_renderBMPE[0] = TRUE;
                    g_renderBMPE[1] = FALSE;
                    g_renderInverse = TRUE;
                    break;
                default:
                    printf ("UI style %d is unknown.\n", style);
                    assert(FALSE);
            }
        }
	}
	else
	{
        // FIXME
        assert(FALSE);
#if 0
		switch (style)
		{
			case UI_TEXT_STYLE_TEXT:
				UI_printf ( CSI "%dm", UI_STYLE_NORMAL);
				break;
			case UI_TEXT_STYLE_KEYWORD:
				UI_printf ( CSI "%dm", UI_STYLE_BOLD);
				UI_printf ( CSI "%dm", UI_WORKBENCH_BLACK);
				break;
			case UI_TEXT_STYLE_COMMENT:
				UI_printf ( CSI "%dm", UI_STYLE_ITALICS);
				UI_printf ( CSI "%dm", UI_WORKBENCH_BLUE);
				break;
			case UI_TEXT_STYLE_INVERSE:
				UI_printf ( CSI "%dm", UI_STYLE_INVERSE);
				break;
			default:
				assert(FALSE);
		}
#endif
	}
}

void UI_beginLine (uint16_t row)
{
    //LOG_printf (LOG_DEBUG, "ui_amiga: beginLine row=%d\n", row);
    if (g_renderRTG)
    {
        assert (!g_bpos);
        Move (g_rp, g_OffLeft, g_OffTop + (row-1) * g_fontHeight + g_rp->TxBaseline);
    }
    else
    {
        if (g_cursorVisible)
            drawCursor();
        g_renderBMcurCol = 0;
        g_renderBMcurRow = row;
        g_renderBMPtr[0] = g_renderBMPlanes[0] + ((row-1) * g_fontHeight + g_BMOffTop) * g_renderBMBytesPerRow;
        g_renderBMPtr[1] = g_renderBMPlanes[1] + ((row-1) * g_fontHeight + g_BMOffTop) * g_renderBMBytesPerRow;
        memset (g_renderBMPtr[0], g_renderInverse ? 0xFF : 0x00, g_renderBMBytesPerRow*g_fontHeight);
        memset (g_renderBMPtr[1], g_renderInverse ? 0xFF : 0x00, g_renderBMBytesPerRow*g_fontHeight);
        if (g_cursorVisible)
            drawCursor();
    }
}

void UI_putc(char c)
{
    if (g_renderRTG)
    {
        g_outbuf[g_bpos++] = c;
        if (g_bpos >= BUFSIZE)
            UI_flush();
    }
    else
    {
        if (g_renderBMcurCol >= g_renderBMmaxCols)
            return;
        g_renderBMcurCol++;

        if (g_cursorVisible)
            drawCursor();
        //printf ("painting char %d (%c)\n", c, c);

        UBYTE ci = c;
        UBYTE *dst0 = g_renderBMPtr[0];
        UBYTE *dst1 = g_renderBMPtr[1];
        //printf ("ci=%d (%c) bl=%d byl=%d bs=%d\n", ci, ci, bl, byl, bs);
        for (UBYTE y=0; y<8; y++)
        {
            UBYTE fd = g_fontData[ci][y];
            if (g_renderInverse)
                fd = ~fd;
            *dst0 = g_renderBMPE[0] ? fd : 0;
            *dst1 = g_renderBMPE[1] ? fd : 0;
            dst0 += g_renderBMBytesPerRow;
            dst1 += g_renderBMBytesPerRow;
        }
        g_renderBMPtr[0]++;
        g_renderBMPtr[1]++;

        if (g_cursorVisible)
            drawCursor();
    }
}

void UI_putstr(char *s)
{
    while (*s)
        UI_putc (*s++);
}

void UI_printf (char* format, ...)
{
    va_list args;
    va_start(args, format);
    UI_vprintf (format, args);
    va_end(args);
}

void UI_vprintf (char* format, va_list args)
{
    static char buf[BUFSIZE];
    vsnprintf (buf, BUFSIZE, format, args);
    UI_putstr(buf);
}

void UI_endLine (void)
{
    if (g_renderRTG)
    {
        UI_flush();
        if (g_cursorVisible)
            drawCursor();
        WORD cp_x = g_rp->cp_x;
        WORD cp_y = g_rp->cp_y-g_rp->TxBaseline;

        WORD max_x = g_win->Width - g_OffRight-1;

        if (cp_x < max_x)
        {
            BYTE FgPen = g_rp->FgPen;
            BYTE DrawMode = g_rp->DrawMode;
            SetAPen(g_rp, 0);
            SetDrMd(g_rp, JAM1);
            RectFill (g_rp, cp_x, cp_y, max_x, cp_y+g_fontHeight-1);
            SetAPen(g_rp, FgPen);
            SetDrMd(g_rp, DrawMode);
        }
        if (g_cursorVisible)
            drawCursor();
    }
}

void UI_setCursorVisible (bool visible)
{
    if ((visible && !g_cursorVisible) || (!visible && g_cursorVisible))
        drawCursor();

    g_cursorVisible = visible;
}

void UI_moveCursor (uint16_t row, uint16_t col)
{
    if (g_cursorVisible)
        drawCursor();
    g_cursorRow = row;
    g_cursorCol = col;
    if (g_cursorVisible)
        drawCursor();
}

struct FileHandle *UI_output (void)
{
    return g_output;
}

int UI_termSignal (void)
{
    return g_termSignalBit;
}

static void returnpacket(struct DosPacket *packet, long res1, long res2)
{
    struct Message *msg;
    struct MsgPort *replyport;

    packet->dp_Res1  = res1;
    packet->dp_Res2  = res2;
    replyport = packet->dp_Port;
    msg = packet->dp_Link;
    packet->dp_Port = g_IOport;
    msg->mn_Node.ln_Name = (char *)packet;
    msg->mn_Node.ln_Succ = NULL;
    msg->mn_Node.ln_Pred = NULL;

    PutMsg(replyport, msg);
}

static struct DosPacket *getpacket(void)
{
    struct Message *msg;
    msg = GetMsg(g_IOport);
    return ((struct DosPacket *)msg->mn_Node.ln_Name);
}

void UI_runIO (void)
{
    ULONG iosig   = 1 << g_IOport->mp_SigBit;
	ULONG termsig = 1 << g_termSignalBit;

    BOOL running = TRUE;
    uint16_t rows, cols;
    UI_getsize(&rows, &cols);
    bool haveLine = FALSE;
    while (running)
    {
        //LOG_printf (LOG_DEBUG, "ui_amiga: UI_runIO: waiting for signal bits %d, %d ...\n", g_IOport->mp_SigBit, g_termSignalBit);
        ULONG signals = Wait(iosig | termsig);
        //LOG_printf (LOG_DEBUG, "ui_amiga: UI_runIO: got signals: 0x%08x\n", signals);

        if (signals & iosig)
		{
			struct DosPacket *packet = getpacket();
			LOG_printf (LOG_DEBUG, "ui_amiga: UI_runIO: got pkg, type=%d\n", packet->dp_Type);

			switch (packet->dp_Type)
			{
				case ACTION_WRITE:
				{
					LONG l = packet->dp_Arg3;
					char *buf = (char *)packet->dp_Arg2;
					//LOG_printf (LOG_DEBUG, "ui_amiga: UI_runIO: ACTION_WRITE, len=%d\n", l);
					for (int i = 0; i<l; i++)
					{
                        if (!haveLine)
                        {
                            UI_scrollUp  (/*fullscreen=*/TRUE);
                            UI_beginLine (rows);
                            haveLine = TRUE;
                        }
                        char c = buf[i];
                        if (c=='\n')
                        {
                            UI_endLine ();
                            haveLine = FALSE;
                        }
                        else
                        {
                            UI_putc(c);
                        }
					}
					UI_flush();
					returnpacket (packet, l, packet->dp_Res2);
					break;
				}
				default:
					//LOG_printf (LOG_DEBUG, "ui_amiga: UI_runIO: rejecting unknown packet type\n");
					returnpacket (packet, FALSE, ERROR_ACTION_NOT_KNOWN);
			}
		}
        else
        {
            if (signals & termsig)
            {
                running = FALSE;
            }
        }
	}
    if (haveLine)
        UI_endLine   ();
}

void UI_bell (void)
{
    DisplayBeep (g_screen);
}

void UI_eraseDisplay (void)
{
    Move (g_rp, 0, 0);
    ClearScreen(g_rp);
}

void UI_setColorScheme (int scheme)
{
    //LOG_printf (LOG_DEBUG, "UI_setColorScheme(%d)\n", scheme);
    OPT_prefSetInt (OPT_PREF_COLORSCHEME, scheme);
    if (g_screen)
    {
        UI_theme_t *theme = &g_themes[scheme];
        for (uint16_t i=0; i<8; i++)
        {
            UBYTE r = theme->palette[i]>>8;
            UBYTE g = (theme->palette[i]>>4) & 0xf;
            UBYTE b = theme->palette[i] & 0xf;
            // LOG_printf (LOG_DEBUG, "UI_setColorScheme(%d): %d -> %d/%d/%d\n", scheme, i, r, g, b);
            SetRGB4 (&g_screen->ViewPort, i, r, g, b);
        }
    }
}

void UI_setCustomScreen (bool enabled)
{
    OPT_prefSetInt (OPT_PREF_CUSTOMSCREEN, enabled ? 1: 0);
}

bool UI_isCustomScreen (void)
{
    return OPT_prefGetInt (OPT_PREF_CUSTOMSCREEN);
}

void UI_setScrollArea (uint16_t row_start, uint16_t row_end)
{
    g_scrollStart = row_start;
    g_scrollEnd   = row_end;
}

void UI_scrollUp (bool fullscreen)
{
    if (g_cursorVisible)
        drawCursor();
    WORD min_x = g_OffLeft;
    WORD min_y = g_OffTop + (g_scrollStart-1)*g_fontHeight;
    WORD max_x = g_win->Width - g_OffRight-1;
    WORD max_y = fullscreen ? g_win->Height-1 : g_scrollEnd*g_fontHeight-1;
#if 0
    printf ("ScrollRaster g_screen: %d x %d, rows=%d\n", g_screen->Width, g_screen->Height, g_screen->BitMap.Rows);
    printf ("ScrollRaster ViewPort: %d x %d\n", g_screen->ViewPort.DWidth, g_screen->ViewPort.DHeight);
    printf ("ScrollRaster g_win: (%d/%d)-(%d/%d)\n", g_win->LeftEdge, g_win->TopEdge, g_win->Width, g_win->Height);
    printf ("ScrollRaster (%d/%d)-(%d/%d) g_scrollStart=%d, g_scrollEnd=%d\n", min_x, min_y, max_x, max_y, g_scrollStart, g_scrollEnd);
    if (fullscreen)
    {
        BYTE FgPen = g_rp->FgPen;
        BYTE DrawMode = g_rp->DrawMode;
        //struct RastPort *rp = &g_screen->RastPort;
        struct RastPort *rp = g_rp;
        SetAPen(rp, 2);
        SetDrMd(rp, JAM1);
        //RectFill (g_rp, min_x, min_y, max_x, max_y);
        Move(rp, min_x, min_y); Draw(rp, max_x, max_y);
        //Move(rp, 0, 0); Draw(rp, 639, 511-2);
        //Move(rp, 0, 0); Draw(rp, 639, 511-4);
        SetAPen(rp, FgPen);
        SetDrMd(rp, DrawMode);

        //UBYTE *p = g_screen->BitMap.Planes[1] + 507*g_renderBMBytesPerRow;
        //for (uint16_t i = 0; i<80; i++)
        //    *p++ = 0xff;

        //ScrollRaster(g_rp, 0, g_fontHeight, min_x, min_y, max_x, max_y);
    }
    else
    {
        ScrollRaster(g_rp, 0, g_fontHeight, min_x, min_y, max_x, max_y);
    }
#else
    ScrollRaster(g_rp, 0, g_fontHeight, min_x, min_y, max_x, max_y);
#endif
    if (g_cursorVisible)
        drawCursor();
}

void UI_scrollDown (void)
{
    if (g_cursorVisible)
        drawCursor();
    WORD max_x = g_win->Width - g_OffRight-1;
    ScrollRaster(g_rp, 0, -g_fontHeight, g_OffLeft, g_OffTop + (g_scrollStart-1)*g_fontHeight, max_x, (g_scrollEnd-1)*g_fontHeight-1);
    if (g_cursorVisible)
        drawCursor();
}

// FIXME: implement size change callback

bool UI_getsize(uint16_t *rows, uint16_t *cols)
{
    uint16_t w = g_win->Width  - g_OffLeft - g_OffRight;
    uint16_t h = g_win->Height - g_OffTop  - g_OffBottom;
    *cols = w / 8;
    *rows = h / g_fontHeight;

    /* range checks */

    if (*cols < UI_MIN_COLUMNS)
        *cols = UI_MIN_COLUMNS;
    if (*cols > UI_MAX_COLUMNS)
        *cols = UI_MAX_COLUMNS;

    if (*rows < UI_MIN_ROWS)
        *rows = UI_MIN_ROWS;
    if (*rows > UI_MAX_ROWS)
        *rows = UI_MAX_ROWS;

    LOG_printf (LOG_DEBUG, "UI_getsize: w=%d, h=%d -> rows=%d, cols=%d\n", w, h, *rows, *cols);

    return TRUE;
}

void UI_onSizeChangeCall (UI_size_cb cb, void *user_data)
{
    g_size_cb = cb;
    g_size_cb_user_data = user_data;
}

void UI_onKeyCall (UI_key_cb cb, void *user_data)
{
    g_key_cb           = cb;
    g_key_cb_user_data = user_data;
}

uint16_t UI_EZRequest (char *body, char *gadgets)
{
	ULONG tags[] = { RTEZ_ReqTitle, (ULONG)"AQB", RT_Window, (ULONG) g_win, TAG_END };
	ULONG res = rtEZRequestA (body, gadgets, /*reqinfo=*/NULL, /*argarray=*/NULL, (struct TagItem *)tags);
	LOG_printf (LOG_DEBUG, "rtEZRequestA result: %ld\n", res);
	return res;
}

char *UI_FileReq  (char *title)
{
    assert(FALSE);
}

typedef enum { esWait, esGet } eventState;

static uint16_t nextEvent(void)
{
    static eventState state = esWait;

    ULONG windowsig  = 1 << g_win->UserPort->mp_SigBit;
    uint16_t res = KEY_NONE;
    ULONG signals;

    switch (state)
    {
        case esWait:
            //LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): Wait...\n");
            signals = Wait(windowsig);
            if (!(signals & windowsig))
                break;
            state = esGet;
            /* fall trough */
        case esGet:
            {
                //LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): GetMsg...\n");
                struct IntuiMessage *winmsg;
                winmsg = (struct IntuiMessage *)GetMsg(g_win->UserPort);
                if (winmsg)
                {
                    state = esGet;
                    //LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): got a message, class=%d\n", winmsg->Class);
                    switch (winmsg->Class)
				    {
                        case IDCMP_CLOSEWINDOW:
                            res = KEY_CLOSE;
				    		break;

				    	case IDCMP_MENUPICK:
				    	{
				    		UWORD menuNumber = winmsg->Code;
                            //LOG_printf (LOG_DEBUG, "ui_amiga: menu picked, menuNumber=%d\n", menuNumber);
				    		while ((menuNumber != MENUNULL))
				    		{
				    			struct MenuItem *item = ItemAddress(g_menuStrip, menuNumber);

				    			UWORD menuNum = MENUNUM(menuNumber);
				    			UWORD itemNum = ITEMNUM(menuNumber);
				    			UWORD subNum  = SUBNUM(menuNumber);
                                uint32_t k = (uint32_t) GTMENUITEM_USERDATA(item);
				    			LOG_printf (LOG_DEBUG, "ui_amiga: menu picked menuNum=%d, itemNum=%d, subNum=%d, userData=%d\n", menuNum, itemNum, subNum, k);
                                if (k)
                                    res = (uint16_t)k;

				    			menuNumber = item->NextSelect;
				    		}
				    		break;
				    	}

				    	case IDCMP_RAWKEY:
                        {
                            //LOG_printf (LOG_DEBUG, "RAWKEY EVENT: code=%d, qualifier=%d\n", winmsg->Code, winmsg->Qualifier);

                            static struct InputEvent ievent;
                            static UBYTE kbuffer[16];
                            ievent.ie_Class            = IECLASS_RAWKEY;
                            ievent.ie_Code             = winmsg->Code;
                            ievent.ie_Qualifier        = winmsg->Qualifier;
                            ievent.ie_position.ie_addr = *((APTR*)winmsg->IAddress);

                            USHORT nc = RawKeyConvert(&ievent, kbuffer, 15, /*kmap=*/NULL);
                            //LOG_printf (LOG_DEBUG, " -> RawKeyConvert nc=%d, buf=\n", nc);
                            //for (int i=0; i<nc; i++)
                            //{
                            //    LOG_printf (LOG_DEBUG, " 0x%02x[%c]", kbuffer[i], kbuffer[i]);
                            //}
                            //LOG_printf(LOG_DEBUG, "\n");
                            switch (nc)
                            {
                                case 1:
                                    switch (kbuffer[0])
                                    {
                                        case 0x14: res = KEY_GOTO_BOF; break;
                                        case 0x02: res = KEY_GOTO_EOF; break;
                                        default: res = (UWORD) kbuffer[0];
                                    }
                                    break;
                                case 2:
                                    switch (kbuffer[0])
                                    {
                                        case 0x9b:
                                            switch (kbuffer[1])
                                            {
                                                case 0x41: if (winmsg->Qualifier & 8) res = KEY_GOTO_BOF ; else res = KEY_CURSOR_UP  ; break;
                                                case 0x42: if (winmsg->Qualifier & 8) res = KEY_GOTO_EOF ; else res = KEY_CURSOR_DOWN; break;
                                                case 0x44: res = KEY_CURSOR_LEFT ; break;
                                                case 0x43: res = KEY_CURSOR_RIGHT; break;
                                                case 0x54: res = KEY_PAGE_UP     ; break;
                                                case 0x53: res = KEY_PAGE_DOWN   ; break;
                                            }
                                    }
                                    break;
                                case 3:
                                    switch (kbuffer[0])
                                    {
                                        case 0x9b:
                                            if (kbuffer[2]==0x7e)
                                            {
                                                switch (kbuffer[1])
                                                {
                                                    case 0x30: res = KEY_F1 ; break;
                                                    case 0x31: res = KEY_F2 ; break;
                                                    case 0x32: res = KEY_F3 ; break;
                                                    case 0x33: res = KEY_F4 ; break;
                                                    case 0x34: res = KEY_F5 ; break;
                                                    case 0x35: res = KEY_F6 ; break;
                                                    case 0x36: res = KEY_F7 ; break;
                                                    case 0x37: res = KEY_F8 ; break;
                                                    case 0x38: res = KEY_F9 ; break;
                                                    case 0x39: res = KEY_F10; break;
                                                }
                                            }
                                            else
                                            {
                                                if (kbuffer[1]==0x20)
                                                {
                                                    switch (kbuffer[2])
                                                    {
                                                        case 0x41: res = KEY_HOME; break;
                                                        case 0x40: res = KEY_END ; break;
                                                    }
                                                }
                                            }
                                    }
                                    break;
                            }
                            break;
                        }
                    }
                    ReplyMsg((struct Message *)winmsg);
                }
                else
                {
                    //LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): got no message\n");
                    state = esWait;
                }
            }
            break;
    } /* switch (state) */

    //LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): done, res=%d\n", res);
    return res;
}
uint16_t UI_waitkey (void)
{
    while (TRUE)
    {
        uint16_t k = nextEvent();
        if (k!=KEY_NONE)
            return k;
    }
}

void UI_deinit(void)
{
    if (g_win)
	{
		ClearMenuStrip(g_win);
		if (g_menuStrip)
			FreeMenus (g_menuStrip);
        CloseWindow(g_win);
	}
	if (g_vi)
		FreeVisualInfo(g_vi);
	if (g_screen)
		CloseScreen (g_screen);
    if (g_IOport)
        delete_port(g_IOport);
    if (g_font)
        CloseFont(g_font);
    if (ConsoleDevice)
        CloseDevice((struct IORequest *)&console_ioreq);
    if (ReqToolsBase)
        CloseLibrary((struct Library *)ReqToolsBase);
    if (g_termSignalBit != -1)
        FreeSignal (g_termSignalBit);
}

static void loadAndConvertTextFont(void)
{
    // load and unpack text font

    static struct TextAttr fontattr = { (STRPTR)"topaz.font", 8, 0, 0 };
    g_font = OpenFont(&fontattr);
    if (!g_font)
        cleanexit("Can't open topaz.font size 8!", RETURN_FAIL);

    for (UWORD ci=0; ci<256; ci++)
        for (UBYTE y=0; y<8; y++)
            g_fontData[ci][y] = 0;

    UWORD *pCharLoc = g_font->tf_CharLoc;
    for (UBYTE ci=g_font->tf_LoChar; ci<g_font->tf_HiChar; ci++)
    {
        UWORD bl = *pCharLoc;
        UWORD byl = bl >> 3;
        BYTE bitl = bl & 3;
        pCharLoc++;
        UWORD bs = *pCharLoc;
        pCharLoc++;
        //printf ("ci=%d (%c) bl=%d byl=%d bs=%d\n", ci, ci, bl, byl, bs);
        for (UBYTE y=0; y<8; y++)
        {
            char *p = g_font->tf_CharData;
            p += y*g_font->tf_Modulo + byl;
            BYTE bsc = bs;
            BYTE bitlc = 7-bitl;
            for (BYTE x=7; x>=0; x--)
            {
                if (*p & (1<<bitlc))
                    g_fontData[ci][y] |= (1<<x);
                bsc--;
                if (!bsc)
                    break;
                bitlc--;
                if (bitlc<0)
                {
                    bitlc = 7;
                    p++;
                }
            }
        }
    }
}

bool UI_init (void)
{
    SysBase = *(APTR *)4L;

    // check library versions

    if ( ((struct Library *)IntuitionBase)->lib_Version < 37)
         cleanexit("intuition library is too old, need at least V37", RETURN_FAIL);
    if ( ((struct Library *)GfxBase)->lib_Version < 37)
         cleanexit("graphics library is too old, need at least V37", RETURN_FAIL);
    if (!(ReqToolsBase = (struct ReqToolsBase *) OpenLibrary ((STRPTR)REQTOOLSNAME, REQTOOLSVERSION)))
         cleanexit("Can't open reqtools.library", RETURN_FAIL);
    if (OpenDevice((STRPTR)"console.device", -1, (struct IORequest *)&console_ioreq,0))
         cleanexit("Can't open console.device", RETURN_FAIL);
    ConsoleDevice = (struct Device *)console_ioreq.io_Device;

    struct Screen *sc = LockPubScreen (NULL); // default public screen
    if (!sc)
         cleanexit("Failed to lock default public screen", RETURN_FAIL);

	// determine screen size
	ULONG mid = GetVPModeID(&sc->ViewPort);

	struct DimensionInfo di;
	if (!GetDisplayInfoData(NULL, (APTR)&di, sizeof(di), DTAG_DIMS, mid))
		 cleanexit("Failed to retrieve display info data", RETURN_FAIL);

	//printf ("ui: workbench screen size is %d x %d (TxtOScan: (%d / %d) - (%d / %d)\n", (int)sc->Width, (int)sc->Height,
	//        (int)di.TxtOScan.MinX, (int)di.TxtOScan.MinY, (int)di.TxtOScan.MaxX, (int)di.TxtOScan.MaxY);

	WORD maxW = di.TxtOScan.MaxX - di.TxtOScan.MinX + 1;
	WORD maxH = di.TxtOScan.MaxY - di.TxtOScan.MinY + 1;

	WORD visWidth = sc->Width > maxW ? maxW : sc->Width;
	WORD visHeight = sc->Height > maxH ? maxH : sc->Height;

	if (!OPT_prefGetInt (OPT_PREF_CUSTOMSCREEN))
	{
		// open a full screen window

		if (!(g_win = OpenWindowTags(NULL,
                                     WA_Width,         visWidth,
									 WA_Height,        visHeight-g_screen->BarHeight-1,
								     WA_IDCMP,         IDCMP_MENUPICK | IDCMP_RAWKEY,
                                     WA_SizeGadget,    TRUE,
                                     WA_DragBar,       TRUE,
                                     WA_DepthGadget,   TRUE,
                                     WA_CloseGadget,   TRUE,
                                     WA_Activate,      TRUE,
									 WA_SimpleRefresh, TRUE,
                                     WA_MaxWidth,      visWidth,
                                     WA_MaxHeight,     visHeight)))
			 cleanexit("Can't open window", RETURN_FAIL);

        g_OffLeft   = g_win->BorderLeft;
        g_OffRight  = g_win->BorderRight;
        g_OffTop    = g_win->BorderTop;
        g_OffBottom = g_win->BorderBottom;

        g_renderRTG = TRUE;
	}
	else
	{
		// open a custom screen that is a clone of the public screen, but has 8 colors and our font

        UI_theme_t *theme = &g_themes[OPT_prefGetInt (OPT_PREF_COLORSCHEME)];

		g_screen = OpenScreenTags(NULL,
			                      SA_Width,      visWidth,
			                      SA_Height,     visHeight,
			                      SA_Depth,      3,
			                      SA_Overscan,   OSCAN_TEXT,
			                      SA_AutoScroll, TRUE,
			                      SA_DisplayID,  mid,
			                      SA_Title,      (ULONG) (STRPTR) "AQB Screen",
                                  SA_Pens,       (ULONG) theme->pens3d,
			                      TAG_END);
        //printf ("visWidth=%d, visHeight=%d\n", visWidth, visHeight);
		if (!g_screen)
			 cleanexit("Can't open screen", RETURN_FAIL);

		UI_setColorScheme(OPT_prefGetInt (OPT_PREF_COLORSCHEME));

		if (!(g_win = OpenWindowTags(NULL,
									 WA_Top,           g_screen->BarHeight+1,
                                     WA_Width,         visWidth,
									 WA_Height,        visHeight-g_screen->BarHeight-1,
								     WA_IDCMP,         IDCMP_MENUPICK | IDCMP_RAWKEY,
								     WA_CustomScreen,  (ULONG) g_screen,
									 WA_Backdrop,      TRUE,
									 WA_SimpleRefresh, TRUE,
									 WA_Activate,      TRUE,
								     WA_Borderless,    TRUE)))
			 cleanexit("Can't open window", RETURN_FAIL);

        g_OffLeft   = 0;
        g_OffRight  = 0;
        g_OffTop    = 0;
        g_BMOffTop  = g_screen->BarHeight+1;
        g_OffBottom = 0;

        // detect RTG screen

        if ( ((struct Library *)GfxBase)->lib_Version >= 39)
        {
            ULONG attr = GetBitMapAttr(&g_screen->BitMap, BMA_FLAGS);
            //printf ("screen bitmap attrs: 0x%08lx BMF_STANDARD=0x%08lx\n", attr, BMF_STANDARD);
            if (!(attr & BMF_STANDARD))
            {
                //printf ("RTG screen detected.\n");
                g_renderRTG = TRUE;
            }
	    }

    }

    UnlockPubScreen(NULL, sc);

    g_rp = g_win->RPort;

    if (!g_renderRTG)
    {
        loadAndConvertTextFont();

        g_renderBMPtr[0] = g_renderBMPlanes[0] = g_screen->BitMap.Planes[0];
        g_renderBMPtr[1] = g_renderBMPlanes[1] = g_screen->BitMap.Planes[1];
        g_renderBMmaxCols = visWidth/8;
        g_renderBMBytesPerRow = g_screen->BitMap.BytesPerRow;
    }

    UI_beginLine (1);

    /* prepare fake i/o filehandles (for IDE console redirection) */

	if ( !(g_output = AllocMem (sizeof(struct FileHandle), MEMF_CLEAR|MEMF_PUBLIC)) )
		cleanexit("failed to allocate memory for output file handle", RETURN_FAIL);

	if (!(g_IOport = create_port ((STRPTR) "aqb_io_port", 0)))
		cleanexit("failed to create i/o port", RETURN_FAIL);

	g_output->fh_Type = g_IOport;
	g_output->fh_Port = 0;
	g_output->fh_Args = (long)g_output;
	g_output->fh_Arg2 = (long)0;

	g_termSignalBit = AllocSignal(-1);
	if (g_termSignalBit == -1)
		cleanexit("failed to allocate signal bit", RETURN_FAIL);

	/*
     * menu
     */

    if (!(g_vi = GetVisualInfo(g_win->WScreen, TAG_END)))
		cleanexit ("failed to get screen visual info", RETURN_FAIL);
	if (!(g_menuStrip = CreateMenus(g_newmenu, TAG_END)))
		cleanexit("failed to create menu", RETURN_FAIL);
	if (!LayoutMenus(g_menuStrip, g_vi, TAG_END))
		cleanexit("failed to layout menu", RETURN_FAIL);
	if (!SetMenuStrip(g_win, g_menuStrip))
		cleanexit("failed to set menu strip", RETURN_FAIL);

	if (OPT_prefGetInt (OPT_PREF_CUSTOMSCREEN))
    {
        struct MenuItem *item = ItemAddress(g_menuStrip, FULLMENUNUM(/*menu=*/2, /*item=*/0, /*sub=*/0));
        item->Flags |= CHECKED;
    }
    struct MenuItem *item = ItemAddress(g_menuStrip, FULLMENUNUM(/*menu=*/2, /*item=*/1, /*sub=*/OPT_prefGetInt (OPT_PREF_COLORSCHEME)));
    item->Flags |= CHECKED;

	return TRUE;
}


static inline void report_key (uint16_t key)
{
    if (key == KEY_NONE)
        return;
    if (g_key_cb)
        g_key_cb (key, g_key_cb_user_data);
}

void UI_run (void)
{
    BOOL running = TRUE;
    while (running)
    {
        uint16_t key = nextEvent();
        if (key == KEY_CLOSE)
            running = FALSE;
        else
            report_key (key);
	}
}

#endif

