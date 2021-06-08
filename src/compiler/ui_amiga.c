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

#include "ui.h"
#include "logger.h"
#include "options.h"

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct IntuitionBase *IntuitionBase;
struct Library              *GadToolsBase;
struct ReqToolsBase         *ReqToolsBase;

#define NEWLIST(l) ((l)->lh_Head = (struct Node *)&(l)->lh_Tail, \
                    /*(l)->lh_Tail = NULL,*/ \
                    (l)->lh_TailPred = (struct Node *)&(l)->lh_Head)

static struct NewWindow g_nw =
{
    0, 0, 640, 200,
    -1,-1,                               /* detailpen, blockpen */
    IDCMP_CLOSEWINDOW | IDCMP_MENUPICK,  /* IDCMP */
    WFLG_DEPTHGADGET   |                 /* window flags */
	WFLG_SIZEGADGET    |
    WFLG_DRAGBAR       |
	WFLG_CLOSEGADGET   |
    WFLG_SMART_REFRESH |
	WFLG_ACTIVATE,
    NULL, NULL,
    (uint8_t *) "AQB",
    NULL,
    NULL,
    100,45,                              /* min width, height */
    1280, 1024,                          /* max width, height */
    WBENCHSCREEN
};

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
static int                g_OffLeft, g_OffRight, g_OffTop, g_OffBottom;
static int                g_termSignalBit = -1;
static APTR               g_vi            = NULL;
static struct Menu       *g_menuStrip     = NULL;
static struct TextFont   *g_font          = NULL;
static UBYTE              g_fontData[256][8];
static struct BitMap      g_renderBM;
static UBYTE             *g_renderBMPlanes[2];
static UBYTE             *g_renderBMPtr[2];
static bool               g_renderBMPE[2]     = { TRUE, FALSE }; // PE: PlaneEnabled
static bool               g_renderInverse     = FALSE;
static uint16_t           g_renderBMcurCol    = 0;
static uint16_t           g_renderBMcurRow    = 0;
static uint16_t           g_renderBMmaxCols   = 80;
static UI_size_cb         g_size_cb           = NULL;
static void              *g_size_cb_user_data = NULL;
static UI_key_cb          g_key_cb            = NULL;
static void              *g_key_cb_user_data  = NULL;
static uint16_t           g_scrollStart       = 0;
static uint16_t           g_scrollEnd         = 10;
static bool               g_cursorVisible     = FALSE;
static uint16_t           g_cursorRow         = 1;
static uint16_t           g_cursorCol         = 1;

#define BUFSIZE 1024

#if 0

#define UI_STYLE_NORMAL     0
#define UI_STYLE_BOLD       1
#define UI_STYLE_ITALICS    3
#define UI_STYLE_UNDERLINE  4
#define UI_STYLE_INVERSE    7

#define UI_WORKBENCH_GREY   30
#define UI_WORKBENCH_BLACK  31
#define UI_WORKBENCH_WHITE  32
#define UI_WORKBENCH_BLUE   33
#endif

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

void UI_setTextStyle (uint16_t style)
{
	if (g_screen)
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
    g_renderBMcurCol = 0;
    g_renderBMcurRow = row;
    g_renderBMPtr[0] = g_renderBMPlanes[0];
    g_renderBMPtr[1] = g_renderBMPlanes[1];
    memset (g_renderBMPtr[0], g_renderInverse ? 0xFF : 0x00, g_renderBM.BytesPerRow*8);
    memset (g_renderBMPtr[1], g_renderInverse ? 0xFF : 0x00, g_renderBM.BytesPerRow*8);
}

void UI_putc(char c)
{
    if (g_renderBMcurCol >= g_renderBMmaxCols)
        return;
    g_renderBMcurCol++;

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
        dst0 += g_renderBM.BytesPerRow;
        dst1 += g_renderBM.BytesPerRow;
    }
    g_renderBMPtr[0]++;
    g_renderBMPtr[1]++;
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

UWORD mypattern[]={0xf0f0, 0xf0f0};

static void drawCursor(void)
{
    uint16_t x = (g_cursorCol-1)*8 + g_OffLeft;
    uint16_t y = (g_cursorRow-1)*8 + g_OffTop;
    SetDrMd (g_rp, COMPLEMENT);
    g_rp->Mask = 3;
    RectFill (g_rp, x, y, x+8, y+8);
#if 0


    //SetAPen (g_rp, 2);
    //SetBPen (g_rp, 3);
    //g_rp->AreaPtrn=mypattern; g_rp->AreaPtSz=1;
    //SetDrMd (g_rp, JAM1 | COMPLEMENT);
    //SetDrMd (g_rp, JAM1);
    //RectFill (g_rp, x, y, x+8, y+8);
    //RectFill (g_rp, 0, 0, 320, 200);
    //RectFill (g_rp, x, y, x+8, y+8);

    //SetDrMd (g_rp, JAM1);
    //for (x=0; x<320; x++)
    //{
    //    SetAPen (g_rp, x%4);
    //    Move (g_rp, 160, 200);
    //    Draw (g_rp, x, 0);
    //}

    //RectFill (g_rp, 20, 20, 40, 40);
    //SetDrMd (g_rp, JAM2);
    //RectFill (g_rp, 30, 30, 50, 50);
    SetDrMd (g_rp, COMPLEMENT);
    g_rp->Mask = 3;
    for (x=0; x<10; x++)
    {
        SetAPen (g_rp, x%4);
        SetBPen (g_rp, x%4);
        RectFill (g_rp, x*10, x*10, x*10+10, x*10+10);
    }

    printf ("drawCursor col=%d, row=%d -> x=%d, y=%d\n", g_cursorCol, g_cursorRow, x, y);
#endif
}

void UI_endLine (void)
{
    if (g_cursorVisible)
        drawCursor();
    BltBitMapRastPort (&g_renderBM, 0, 0, g_rp, 0, (g_renderBMcurRow-1)*8, g_renderBMmaxCols*8, 8, 0xc0);
    if (g_cursorVisible)
        drawCursor();
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
    assert(FALSE); // FIXME
    return NULL;
#if 0
    return g_output;
#endif
}

int UI_termSignal (void)
{
    assert(FALSE); // FIXME
    return 0;
#if 0
    return g_termSignalBit;
#endif
}

#if 0
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
#endif


void UI_runIO (void)
{
    assert(FALSE); // FIXME
#if 0

    ULONG iosig   = 1 << g_IOport->mp_SigBit;
	ULONG termsig = 1 << g_termSignalBit;

    BOOL running = TRUE;
    while (running)
    {
        //LOG_printf (LOG_DEBUG, "term: UI_runIO: waiting for signal bits %d, %d ...\n", g_IOport->mp_SigBit, g_termSignalBit);
        ULONG signals = Wait(iosig | termsig);
        //LOG_printf (LOG_DEBUG, "term: UI_runIO: got signals: 0x%08x\n", signals);

        if (signals & iosig)
		{
			struct DosPacket *packet = getpacket();
			LOG_printf (LOG_DEBUG, "term: UI_runIO: got pkg, type=%d\n", packet->dp_Type);

			switch (packet->dp_Type)
			{
				case ACTION_WRITE:
				{
					LONG l = packet->dp_Arg3;
					char *buf = (char *)packet->dp_Arg2;
					//LOG_printf (LOG_DEBUG, "term: UI_runIO: ACTION_WRITE, len=%d\n", l);
					for (int i = 0; i<l; i++)
					{
                        char c = buf[i];
                        if (c=='\n')
                        {
                            uint16_t rows, cols;
                            UI_getsize(&rows, &cols);
                            UI_scrollUp (/*fullscreen=*/TRUE);
                            UI_moveCursor (rows+1, 1);
                            UI_eraseToEOL ();
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
					//LOG_printf (LOG_DEBUG, "term: UI_runIO: rejecting unknown packet type\n");
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
#endif
}

#if 0

// handle CSI sequences: state machine

//typedef enum {ESC_idle, ESC_esc1, ESC_csi, ESC_tilde, ESC_1,
//              ESC_EVENT} ESC_state_t;


//static ESC_state_t g_esc_state = ESC_idle;
#define MAX_EVENT_BUF 32
#endif

void UI_bell (void)
{
    assert(FALSE); // FIXME
#if 0
    DisplayBeep (g_screen);
#endif
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
    assert(FALSE); // FIXME
#if 0
    if (!fullscreen)
    {
        //LOG_printf (LOG_DEBUG, "scroll up, g_scrollEnd=%d\n", g_scrollEnd);
        UI_printf ( CSI "%dt", g_scrollEnd);
    }
    UI_printf ( CSI "S");
    if (!fullscreen)
    {
        //UI_printf ( CSI "y");
        UI_printf ( CSI "t");
    }
#endif
}

void UI_scrollDown (void)
{
    assert(FALSE); // FIXME
#if 0
    //LOG_printf (LOG_DEBUG, "scroll down, g_scrollEnd=%d\n", g_scrollEnd);
    //UI_printf ( CSI "%dy", g_scrollStart+1);
    UI_printf ( CSI "%dt", g_scrollEnd);
    UI_printf ( CSI "T");
    //UI_printf ( CSI "y");
    UI_printf ( CSI "t");
#endif
}

// FIXME: implement size change callback

bool UI_getsize(uint16_t *rows, uint16_t *cols)
{
    *cols = (g_win->Width  - g_OffLeft - g_OffRight ) / g_rp->Font->tf_XSize;
    *rows = (g_win->Height - g_OffTop  - g_OffBottom) / g_rp->Font->tf_YSize;

    /* range checks */

    if (*cols < UI_MIN_COLUMNS)
        *cols = UI_MIN_COLUMNS;
    if (*cols > UI_MAX_COLUMNS)
        *cols = UI_MAX_COLUMNS;

    if (*rows < UI_MIN_ROWS)
        *rows = UI_MIN_ROWS;
    if (*rows > UI_MAX_ROWS)
        *rows = UI_MAX_ROWS;

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
    assert(FALSE); // FIXME
    return 0;
#if 0
	ULONG tags[] = { RTEZ_ReqTitle, (ULONG)"AQB", RT_Window, (ULONG) g_win, TAG_END };
	ULONG res = rtEZRequestA (body, gadgets, /*reqinfo=*/NULL, /*argarray=*/NULL, (struct TagItem *)tags);
	LOG_printf (LOG_DEBUG, "rtEZRequestA result: %ld\n", res);
	return res;
#endif
}

char *UI_FileReq  (char *title)
{
    assert(FALSE);
}

uint16_t UI_waitkey (void)
{
    assert(FALSE);  // FIXME
    return 0;
    //return nextEvent();
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
    if (g_renderBMPlanes[0])
        FreeMem (g_renderBMPlanes[0], g_renderBM.BytesPerRow * 8);
    if (g_renderBMPlanes[1])
        FreeMem (g_renderBMPlanes[1], g_renderBM.BytesPerRow * 8);
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

    loadAndConvertTextFont();

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

		g_nw.Width     = visWidth;
		g_nw.Height    = visHeight;
        g_nw.MaxWidth  = visWidth;
        g_nw.MaxHeight = visHeight;

		if (!(g_win = OpenWindow(&g_nw)))
			 cleanexit("Can't open window", RETURN_FAIL);

        g_OffLeft   = g_win->BorderLeft;
        g_OffRight  = g_win->BorderRight;
        g_OffTop    = g_win->BorderTop;
        g_OffBottom = g_win->BorderBottom;
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
		if (!g_screen)
			 cleanexit("Can't open screen", RETURN_FAIL);

		UI_setColorScheme(OPT_prefGetInt (OPT_PREF_COLORSCHEME));

		if (!(g_win = OpenWindowTags(NULL,
									 WA_Top,           g_screen->BarHeight+1,
                                     WA_Width,         visWidth,
									 WA_Height,        visHeight-g_screen->BarHeight-1,
								     WA_IDCMP,         IDCMP_MENUPICK,
								     WA_CustomScreen,  (ULONG) g_screen,
									 WA_Backdrop,      TRUE,
									 WA_SimpleRefresh, TRUE,
									 WA_Activate,      TRUE,
								     WA_Borderless,    TRUE)))
			 cleanexit("Can't open window", RETURN_FAIL);

        g_OffLeft   = 0;
        g_OffRight  = 0;
        g_OffTop    = 0;
        g_OffBottom = 0;
	}

    UnlockPubScreen(NULL, sc);

    g_rp = g_win->RPort;

    // setup bg render bitmap and engine

    InitBitMap (&g_renderBM, /*Depth=*/2, visWidth, visHeight);

    if (! (g_renderBMPlanes[0] = AllocMem (g_renderBM.BytesPerRow * 8, MEMF_CHIP)))
        cleanexit ("Can't allocate background BitMap planes", RETURN_FAIL);
    if (! (g_renderBMPlanes[1] = AllocMem (g_renderBM.BytesPerRow * 8, MEMF_CHIP)))
        cleanexit ("Can't allocate background BitMap planes", RETURN_FAIL);
    g_renderBM.Planes[0] = g_renderBMPtr[0] = g_renderBMPlanes[0];
    g_renderBM.Planes[1] = g_renderBMPtr[1] = g_renderBMPlanes[1];

    g_renderBMmaxCols = visWidth/8;
    UI_beginLine (0);

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

static uint16_t nextEvent(void)
{
    // UBYTE ch;

    ULONG windowsig  = 1 << g_win->UserPort->mp_SigBit;

    BOOL running = TRUE;
    while (running)
    {
        ULONG signals = Wait(windowsig);

        if (signals & windowsig)
		{
			struct IntuiMessage *winmsg;
            while (winmsg = (struct IntuiMessage *)GetMsg(g_win->UserPort))
			{
                switch (winmsg->Class)
				{
                    case IDCMP_CLOSEWINDOW:
						running = FALSE;
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
                                return (uint16_t)k;

							menuNumber = item->NextSelect;
						}
						break;
					}

                    default:
						break;
				}
                ReplyMsg((struct Message *)winmsg);
			}
		}
	}

    return KEY_CLOSE;
}

static inline void report_key (uint16_t key)
{
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

