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
#include <clib/asl_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/console_protos.h>
#include <clib/utility_protos.h>
#include <clib/gadtools_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/intuition.h>
#include <inline/graphics.h>
#include <inline/asl.h>
#include <inline/utility.h>
#include <inline/gadtools.h>

#include <libraries/gadtools.h>

#include <clib/console_protos.h>
#include <inline/console.h>

#include "ui.h"
#include "logger.h"
#include "options.h"
#include "amigasupport.h"
#include "ide.h"

//#define LOG_KEY_EVENTS
//#define ENABLE_SCROLL_BENCHMARK

#define BM_HEIGHT 8
#define NUM_VIEWS 3

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct IntuitionBase *IntuitionBase;
extern struct AslBase       *AslBase;
extern struct UtilityBase   *UtilityBase;
struct Library              *GadToolsBase;
struct Device               *ConsoleDevice = NULL;

static struct NewMenu g_newmenu[] =
    {
        { NM_TITLE, (STRPTR) "Project",             0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "New",                 0 , 0, 0, (APTR)KEY_NEW,},
        {  NM_ITEM, (STRPTR) "Open...",   (STRPTR) "O", 0, 0, (APTR)KEY_OPEN,},
        {  NM_ITEM, NM_BARLABEL,                    0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Save",      (STRPTR) "S", 0, 0, (APTR)KEY_SAVE,},
        {  NM_ITEM, (STRPTR) "Save As...",(STRPTR) "A", 0, 0, (APTR)KEY_SAVE_AS,},
        {  NM_ITEM, NM_BARLABEL,                    0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Help...",             0 , 0, 0, (APTR)KEY_HELP,},
        {  NM_ITEM, (STRPTR) "About...",            0 , 0, 0, (APTR)KEY_ABOUT,},
        {  NM_ITEM, NM_BARLABEL,                    0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Quit...",   (STRPTR) "Q", 0, 0, (APTR)KEY_QUIT,},

        { NM_TITLE, (STRPTR) "Edit",                0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Cut",       (STRPTR) "X", 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Copy",      (STRPTR) "C", 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Paste",     (STRPTR) "V", 0, 0, 0,},
        {  NM_ITEM, NM_BARLABEL,                    0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Block",     (STRPTR) "B", 0, 0, (APTR)KEY_BLOCK,},

        { NM_TITLE, (STRPTR) "Find",                0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Find...",   (STRPTR) "F", 0, 0, (APTR)KEY_FIND,},
        {  NM_ITEM, (STRPTR) "Find next", (STRPTR) "N", 0, 0, (APTR)KEY_FIND_NEXT,},

        { NM_TITLE, (STRPTR) "Run",                 0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Compile      F7",     0, 0, 0, (APTR)KEY_F7,},
        {  NM_ITEM, (STRPTR) "Compile+Run  F5",     0, 0, 0, (APTR)KEY_F5,},

        { NM_TITLE, (STRPTR) "Settings",            0 , 0, 0, 0,},
        {  NM_ITEM, (STRPTR) "Colorscheme",         0 , 0, 0, 0,},
        {   NM_SUB, (STRPTR) "Dark",                0 , CHECKIT | MENUTOGGLE,  ~1, (APTR)KEY_COLORSCHEME_0,},
        {   NM_SUB, (STRPTR) "Light",               0 , CHECKIT | MENUTOGGLE,  ~2, (APTR)KEY_COLORSCHEME_1,},
        {  NM_ITEM, (STRPTR) "Font",                0 , 0, 0, 0,},
        {   NM_SUB, (STRPTR) "6",                   0 , CHECKIT | MENUTOGGLE,  ~1, (APTR)KEY_FONT_0,},
        {   NM_SUB, (STRPTR) "8",                   0 , CHECKIT | MENUTOGGLE,  ~2, (APTR)KEY_FONT_1,},
        {   NM_END, NULL, 0 , 0, 0, 0,},
    };

struct UI_view_
{
    WORD                  x, y, w, h;

    BOOL                  visible;

    WORD                  visWidth;
    BOOL                  renderBMPE[3]     ; // PE: PlaneEnabled
    BOOL                  renderBMPEI[3]    ; // PE: PlaneEnabledInverse
    UBYTE                 renderBMcurFG;
    UBYTE                 renderBMcurBG;

    struct BitMap        *renderBM;
    UBYTE                *renderBMPtr[3];
    UBYTE                *renderBMPlanes[3];
    UWORD                 renderBMBytesPerRow;

    WORD                  curLineStart;
    WORD                  curLineCols;
    WORD                  renderBMcurCol;
    WORD                  renderBMcurRow;
    WORD                  renderBMmaxCols;

    BOOL                  cursorVisible;
    WORD                  cursorRow;
    WORD                  cursorCol;

    UI_event_cb           event_cb;
    void                 *event_cb_user_data;

    bool                  scrollable;
    struct Gadget        *scrollbar;
    uint16_t              scrollPos;
};

static struct Window        *g_win               = NULL;
static struct RastPort      *g_rp                = NULL;
static struct MsgPort       *g_debugPort         = NULL;
static struct IOStdReq       console_ioreq;
static struct Menu          *g_menuStrip         = NULL;
static struct Gadget        *g_glist             = NULL; // gadtools context
static struct Gadget        *g_gad               = NULL;
static APTR                  g_vi                = NULL;
static UI_view               g_views[NUM_VIEWS]  = {NULL, NULL, NULL};
static UI_view               g_active_view       = NULL;
static UWORD                 g_fontHeight        = 8;
static UBYTE                 g_curFont           = 1;
static uint16_t              g_theme             = 0;

#include "fonts.h"

struct TextAttr Topaz80 = { (STRPTR)"topaz.font", 8, 0, 0, };
// FIXME
#if 0
#define NL 0

#define REDP 3
#define BLKP 2
#define WHTP 1
#define BLUP 0

static struct IntuiText text = {
  BLUP, WHTP, JAM2,
  58, 5, NL,
  (UBYTE *) "Regular requestor",
  NL
};



static struct Window         g_FindReq;
static struct Gadget        *g_FRGadgetList = NULL;
static struct Gadget        *g_FRFindButton;
#endif

static struct FileRequester *g_ASLFileReq   = NULL;

#define BUFSIZE 1024

typedef struct
{
    char   *name;
    uint8_t fg[13];
    uint8_t bg[13];
} UI_theme_t;

#define NUM_THEMES 2

static UI_theme_t g_themes[NUM_THEMES] = {
    {
        "Dark",
        //  TEXT KEYWORD COMMENT INVERSE DIALOG ANSI0 ANSI1 ANSI2 ANSI3 ANSI4 ANSI5 ANSI6 ANSI7
        {      2,      3,      0,      1,     1,    0,    1,    2,    3,    0,    1,    2,    3  },
        {      1,      1,      1,      0,     3,    1,    1,    1,    1,    1,    1,    1,    1  }
    },
    {
        "Light",
        //  TEXT KEYWORD COMMENT INVERSE DIALOG ANSI0 ANSI1 ANSI2 ANSI3 ANSI4 ANSI5 ANSI6 ANSI7
        {      1,      2,      3,      0,     0,    0,    1,    2,    3,    0,    1,    2,    3  },
        {      0,      0,      0,      1,     3,    0,    0,    0,    0,    0,    0,    0,    0  }
    },
};

static void cleanexit (char *s, uint32_t n)
{
    if (s)
    {
        printf("%s\n", s);
        U_request(NULL, NULL, "OK", "%s", s);
    }
    exit(n);
}

#define _createGadgetTags(kind, prev, x, y, w, h, txt, id, flags, ___tagList, ...) \
    ({_sfdc_vararg _tags[] = { ___tagList, __VA_ARGS__ }; _createGadgetTagList((kind), (prev), (x), (y), (w), (h), (txt), (id), (flags), (const struct TagItem *) _tags); })


static struct Gadget *_createGadgetTagList (ULONG kind, struct Gadget *prev, WORD x, WORD y, WORD w, WORD h,
                                            char *txt, UWORD id, ULONG flags, const struct TagItem *tags)
{
    //_sfdc_vararg tags[] = { ___tagList, __VA_ARGS__ };
    //OpenScreenTagList((___newScreen), (const struct TagItem *) _tags); }

    struct NewGadget ng;

    ng.ng_LeftEdge   = x + g_win->BorderLeft;
    ng.ng_TopEdge    = y + g_win->BorderTop;
    ng.ng_Width      = w;
    ng.ng_Height     = h;
    ng.ng_TextAttr   = &Topaz80;
    ng.ng_VisualInfo = g_vi;
    ng.ng_GadgetText = (STRPTR) txt;
    ng.ng_GadgetID   = id;
    ng.ng_Flags      = flags;

    // FIXME: remove debug code
#if 0
    struct TagItem *ti = FindTagItem (GTST_String, tags);

    LOG_printf (LOG_DEBUG, "_createGadget: GTST_String ti=0x%08lx\n", ti);
    if (ti)
        LOG_printf (LOG_DEBUG, "   ->value=0x%08lx (%s)\n", ti->ti_Data, (char *) ti->ti_Data);

    ti = FindTagItem (GTST_MaxChars, tags);

    LOG_printf (LOG_DEBUG, "_createGadget: GTST_MaxChars ti=0x%08lx -> %ld\n", ti, ti ? ti->ti_Data : 0);
    printf ("_createGadgetTagList: prev=%08lx, x=%d, y=%d, w=%d, h=%d, txt=%s, id=%d, flags=%ld\n", (ULONG) prev, x, y, w, h, txt, id, flags);
#endif

    return CreateGadgetA (kind, prev, &ng, tags);
}

static UI_view UI_View(UBYTE depth, WORD visWidth, BOOL scrollable)
{
    UI_view view = U_poolAlloc(UP_ide, sizeof(*view));

    view->x                 = 0;
    view->y                 = 0;
    view->w                 = 0;
    view->h                 = 0;

    view->visible           = TRUE;

    view->visWidth          = visWidth;
    view->cursorVisible     = FALSE;

    view->event_cb          = NULL;

    view->scrollable        = scrollable;
    view->scrollbar         = NULL;
    view->scrollPos         = 0;


    UI_setTextStyle (view, UI_TEXT_STYLE_TEXT);

    view->renderBM = AllocMem(sizeof(struct BitMap), MEMF_PUBLIC | MEMF_CLEAR);
    if (!view->renderBM)
         cleanexit("Failed to allocate render BitMap struct", RETURN_FAIL);

    InitBitMap(view->renderBM, depth, visWidth, BM_HEIGHT);

    for (uint8_t planeNum = 0; planeNum < depth; planeNum++)
    {
        view->renderBMPtr[planeNum] = view->renderBM->Planes[planeNum] = view->renderBMPlanes[planeNum] = AllocRaster(visWidth, BM_HEIGHT);
        if (!view->renderBM->Planes[planeNum])
            cleanexit ("Failed to allocate render BitMap plane", RETURN_FAIL);
    }

    view->renderBMBytesPerRow = view->renderBM->BytesPerRow;

    return view;
}

UI_view UI_getView (UI_viewId id)
{
    return g_views[id];
}

void UI_getViewSize (UI_view view, int16_t *rows, int16_t *cols)
{
    *cols = view->renderBMmaxCols;
    *rows = view->h/g_fontHeight;
}

void UI_cfgViewScroller (UI_view view, uint16_t top, uint16_t total, uint16_t visible)
{
    assert (view->scrollable);
    view->scrollPos = top;
    GT_SetGadgetAttrs (view->scrollbar, g_win, NULL,
                       GTSC_Top, top, GTSC_Total, total, GTSC_Visible, visible, TAG_END);
}

uint16_t UI_getViewScrollPos (UI_view view)
{
    return view->scrollPos;
}

void UI_setColorScheme (int theme)
{
    //LOG_printf (LOG_DEBUG, "UI_setColorScheme(%d)\n", theme);
    OPT_prefSetInt (OPT_PREF_COLORSCHEME, theme);
    g_theme = theme;
}

static void _view_resize (UI_view view, WORD x, WORD y, WORD w, WORD h)
{
    LOG_printf (LOG_DEBUG, "UI: _view_resize: view=0x%08lx x=%d y=%d w=%d h=%d\n", view, x, y, w, h);

    view->x               = x;
    view->y               = y;
    view->w               = w;
    view->h               = h;
    view->renderBMmaxCols = w/8;
}

static void _updateLayout(void)
{
    LOG_printf (LOG_DEBUG, "UI: _updateLayout starts\n");

    WORD y = g_win->BorderTop;
    WORD w = g_win->Width - g_win->BorderLeft - g_win->BorderRight;

    WORD h = g_win->Height - g_win->BorderTop - g_win->BorderBottom;

    WORD status_h  = g_fontHeight;
    WORD console_h = h / 4;
    WORD editor_h  = g_views[UI_viewConsole]->visible ? h - status_h - console_h : h - status_h;

    if (g_glist)
    {
        RemoveGList (g_win, g_glist, -1);
        FreeGadgets (g_glist);
        g_glist = NULL;
        g_gad = CreateContext (&g_glist);
    }

    RefreshWindowFrame (g_win);

    if (g_views[UI_viewEditor]->visible)
    {
        _view_resize(g_views[UI_viewEditor]   , g_win->BorderLeft, y, w, editor_h);

        if (g_views[UI_viewEditor]->scrollable)
        {
            g_gad = _createGadgetTags (SCROLLER_KIND, g_gad,
                                       g_win->Width - g_win->BorderRight - g_win->BorderLeft,
                                       y-g_win->BorderTop, g_win->BorderRight, editor_h, "V", UI_viewEditor, 0,
                                       GTSC_Total, 1000, GTSC_Visible, 25, GTSC_Arrows, 10, PGA_Freedom, LORIENT_VERT,
                                       GA_RelVerify, TRUE, TAG_END);
            g_views[UI_viewEditor]->scrollbar = g_gad;
        }

        y += editor_h;
    }
    if (g_views[UI_viewStatusBar]->visible)
    {
        _view_resize(g_views[UI_viewStatusBar], g_win->BorderLeft, y, w, status_h);
        y += status_h;
    }
    if (g_views[UI_viewConsole]->visible)
    {
        _view_resize(g_views[UI_viewConsole]  , g_win->BorderLeft, y, w, console_h);
        if (g_views[UI_viewConsole]->scrollable)
        {
            g_gad = _createGadgetTags (SCROLLER_KIND, g_gad,
                                       g_win->Width - g_win->BorderRight - g_win->BorderLeft,
                                       y-g_win->BorderTop, g_win->BorderRight, console_h-8, "V", UI_viewConsole, 0,
                                       GTSC_Total, 1000, GTSC_Visible, 25, GTSC_Arrows, 10, PGA_Freedom, LORIENT_VERT, GA_RelVerify, TRUE, TAG_END);
            g_views[UI_viewConsole]->scrollbar = g_gad;
        }

    }

    for (int i = 0; i<NUM_VIEWS; i++)
    {
        if (g_views[i]->visible && g_views[i]->event_cb)
        {
            LOG_printf (LOG_DEBUG, "UI: calling event cb for view #%d...\n", i);
            g_views[i]->event_cb(g_views[i], KEY_RESIZE, g_views[i]->event_cb_user_data);
        }
    }

    if (g_glist)
    {
        AddGList (g_win, g_glist, 1000, -1, NULL);
        GT_RefreshWindow (g_win, NULL);
        RefreshGadgets (g_glist, g_win, NULL);
        GT_BeginRefresh (g_win);
        GT_EndRefresh (g_win, TRUE);
    }

    LOG_printf (LOG_DEBUG, "UI: _updateLayout done\n");
}

void UI_setFont (int font)
{
    OPT_prefSetInt (OPT_PREF_FONT, font);
    g_curFont = font;
    g_fontHeight = font ? 8 : 6;

    _updateLayout();
}

void UI_bell (void)
{
    DisplayBeep (NULL);
}

void UI_toFront(void)
{
    WBenchToFront();
    WindowToFront(g_win);
    ActivateWindow(g_win);
}

typedef enum { esWait, esGetWin, esGetDebug } eventState;

static uint16_t nextEvent(void)
{
    static eventState state = esWait;

    ULONG windowsig = 1 << g_win->UserPort->mp_SigBit;
	ULONG debugsig  = 1 << g_debugPort->mp_SigBit;
    uint16_t res = KEY_NONE;
    static ULONG signals;

    switch (state)
    {
        case esWait:
            //LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): Wait...\n");
            signals = Wait(windowsig|debugsig);
            state = esGetWin;
            /* fall trough */
        case esGetWin:
            if (signals & windowsig)
            {
                //LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): GetIMsg...\n");
                struct IntuiMessage *winmsg = GT_GetIMsg(g_win->UserPort);
                if (winmsg)
                {
                    LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): got a message, class=0x%x\n", winmsg->Class);
                    switch (winmsg->Class)
				    {
                        case IDCMP_REFRESHWINDOW:
                            GT_BeginRefresh (g_win);
                            GT_EndRefresh (g_win, TRUE);
				    		break;

                        case IDCMP_CLOSEWINDOW:
                            res = KEY_QUIT;
				    		break;

                        case IDCMP_NEWSIZE:
                            _updateLayout();
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
#ifdef LOG_KEY_EVENTS
                            LOG_printf (LOG_DEBUG, " -> RawKeyConvert nc=%d, buf=\n", nc);
                            for (int i=0; i<nc; i++)
                            {
                                LOG_printf (LOG_DEBUG, " 0x%02x[%c]", kbuffer[i], kbuffer[i]);
                            }
                            LOG_printf(LOG_DEBUG, "\n");
#endif
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

                        case IDCMP_GADGETUP:
                        {
                            LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): IDCMP_GADGETUP, code=%d\n", winmsg->Code);
                            struct Gadget *gad = (struct Gadget *)winmsg->IAddress;
                            switch (gad->GadgetID)
                            {
                                case UI_viewEditor:
                                case UI_viewStatusBar:
                                case UI_viewConsole:
                                {
                                    UI_view view = g_views[gad->GadgetID];
                                    view->scrollPos = winmsg->Code;
                                    if (view->visible && view->event_cb)
                                        view->event_cb(view, KEY_SCROLLV, view->event_cb_user_data);
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    GT_ReplyIMsg(winmsg);
                    break;
                }
                else
                {
                    //LOG_printf (LOG_DEBUG, "ui_amiga: nextEvent(): got no message\n");
                    state = esGetDebug;
                }
            }
            else
            {
                    state = esGetDebug;
            }
            /* fall trough */
        case esGetDebug:
            if (signals & debugsig)
            {
                res = DEBUG_handleMessages();
            }
            state = esWait;
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

static void _setTextColor (UI_view view, uint8_t fg, uint8_t bg)
{
    switch (fg)
    {
        case 0: view->renderBMPE[0] = FALSE; view->renderBMPE[1] = FALSE; view->renderBMPE[2] = FALSE; break;
        case 1: view->renderBMPE[0] =  TRUE; view->renderBMPE[1] = FALSE; view->renderBMPE[2] = FALSE; break;
        case 2: view->renderBMPE[0] = FALSE; view->renderBMPE[1] =  TRUE; view->renderBMPE[2] = FALSE; break;
        case 3: view->renderBMPE[0] =  TRUE; view->renderBMPE[1] =  TRUE; view->renderBMPE[2] = FALSE; break;
        case 4: view->renderBMPE[0] = FALSE; view->renderBMPE[1] = FALSE; view->renderBMPE[2] =  TRUE; break;
        case 5: view->renderBMPE[0] =  TRUE; view->renderBMPE[1] = FALSE; view->renderBMPE[2] =  TRUE; break;
        case 6: view->renderBMPE[0] = FALSE; view->renderBMPE[1] =  TRUE; view->renderBMPE[2] =  TRUE; break;
        case 7: view->renderBMPE[0] =  TRUE; view->renderBMPE[1] =  TRUE; view->renderBMPE[2] =  TRUE; break;
        default: assert(FALSE);
    }

    switch (bg)
    {
        case 0: view->renderBMPEI[0] = FALSE; view->renderBMPEI[1] = FALSE; view->renderBMPEI[2] = FALSE; break;
        case 1: view->renderBMPEI[0] =  TRUE; view->renderBMPEI[1] = FALSE; view->renderBMPEI[2] = FALSE; break;
        case 2: view->renderBMPEI[0] = FALSE; view->renderBMPEI[1] =  TRUE; view->renderBMPEI[2] = FALSE; break;
        case 3: view->renderBMPEI[0] =  TRUE; view->renderBMPEI[1] =  TRUE; view->renderBMPEI[2] = FALSE; break;
        case 4: view->renderBMPEI[0] = FALSE; view->renderBMPEI[1] = FALSE; view->renderBMPEI[2] =  TRUE; break;
        case 5: view->renderBMPEI[0] =  TRUE; view->renderBMPEI[1] = FALSE; view->renderBMPEI[2] =  TRUE; break;
        case 6: view->renderBMPEI[0] = FALSE; view->renderBMPEI[1] =  TRUE; view->renderBMPEI[2] =  TRUE; break;
        case 7: view->renderBMPEI[0] =  TRUE; view->renderBMPEI[1] =  TRUE; view->renderBMPEI[2] =  TRUE; break;
        default: assert(FALSE);
    }

    view->renderBMcurFG = fg;
    view->renderBMcurBG = bg;
}

void UI_setTextStyle (UI_view view, uint16_t style)
{
    _setTextColor (view, g_themes[g_theme].fg[style], g_themes[g_theme].bg[style]);
}

void UI_beginLine (UI_view view, uint16_t row, uint16_t col_start, uint16_t cols)
{
    // LOG_printf (LOG_DEBUG,
    //             "ui_amiga: beginLine row=%d, col_start=%d, cols=%d\n",
    //             row, col_start, cols);

    assert (cols <= view->renderBMmaxCols);

    view->curLineStart = col_start;
    view->curLineCols  = cols;
    //LOG_printf (LOG_DEBUG, "ui_amiga: beginLine view->curLineCols=%d\n", view->curLineCols);
    view->renderBMcurCol = col_start-1;
    view->renderBMcurRow = row;
    for (uint8_t d = 0; d<view->renderBM->Depth; d++)
        view->renderBMPtr[d] = view->renderBMPlanes[d];
    for (uint16_t r = 0; r<g_fontHeight; r++)
    {
        for (uint16_t d = 0; d<view->renderBM->Depth; d++)
            memset (view->renderBMPtr[d] + r*view->renderBMBytesPerRow, view->renderBMPEI[d] ? 0xff : 0x00, view->curLineCols);
    }
}

static void _drawCursor(UI_view view)
{
    if (!view->visible)
        return;

    uint16_t x = (view->cursorCol-1)*8 + view->x;
    uint16_t y = (view->cursorRow-1)*g_fontHeight + view->y;
    //LOG_printf (LOG_DEBUG, "ui_amiga: drawCursor view->cursorCol=%d, view->cursorRow=%d, x=%d, y=%d\n", view->cursorCol, view->cursorRow, x, y);
    BYTE DrawMode = g_rp->DrawMode;
    SetDrMd (g_rp, COMPLEMENT);
    g_rp->Mask = 3;
    RectFill (g_rp, x, y, x+7, y+g_fontHeight-1);
    SetDrMd (g_rp, DrawMode);
}

void UI_putc(UI_view view, char c)
{
    if (!view->visible)
        return;

    if (view->renderBMcurCol >= view->renderBMmaxCols)
        return;

    view->renderBMcurCol++;

    if (view->cursorVisible && view->visible && (view==g_active_view) )
        _drawCursor(view);
    //printf ("painting char %d (%c)\n", c, c);

    UBYTE ci = c;
    UBYTE *dst[3];
    for (uint8_t planeNum = 0; planeNum < view->renderBM->Depth; planeNum++)
        dst[planeNum] = view->renderBMPtr[planeNum];

    //printf ("ci=%d (%c) bl=%d byl=%d bs=%d\n", ci, ci, bl, byl, bs);
    for (UBYTE y=0; y<g_fontHeight; y++)
    {
        UBYTE fd = g_fontData[g_curFont][ci][y];
        for (uint8_t planeNum = 0; planeNum < view->renderBM->Depth; planeNum++)
        {
            *dst[planeNum]  = view->renderBMPE[planeNum] ? fd : 0;
            if (view->renderBMPEI[planeNum])
                *dst[planeNum] |= ~fd;
            dst[planeNum] += view->renderBMBytesPerRow;
        }
    }
    for (uint8_t planeNum = 0; planeNum < view->renderBM->Depth; planeNum++)
        view->renderBMPtr[planeNum]++;

    if (view->cursorVisible && view->visible && (view==g_active_view) )
        _drawCursor(view);
}

void UI_putstr (UI_view view, char *s)
{
    while (*s)
        UI_putc (view, *s++);
}

void UI_printf (UI_view view, char* format, ...)
{
    va_list args;
    va_start(args, format);
    UI_vprintf (view, format, args);
    va_end(args);
}

void UI_vprintf (UI_view view, char* format, va_list args)
{
    static char buf[BUFSIZE];
    vsnprintf (buf, BUFSIZE, format, args);
    UI_putstr(view, buf);
}

void UI_endLine (UI_view view)
{
    if (!view->visible)
        return;

    if (view->cursorVisible && (view==g_active_view) )
        _drawCursor(view);
    BltBitMapRastPort (view->renderBM, 0, 0, g_rp, (view->curLineStart-1)*8+view->x, (view->renderBMcurRow-1)*g_fontHeight+view->y, view->curLineCols*8, g_fontHeight, 0xc0);
    if (view->cursorVisible && (view==g_active_view) )
        _drawCursor(view);
}

void UI_setCursorVisible (UI_view view, bool visible)
{
    if (view->visible && g_active_view == view)
    {
        if ((visible && !view->cursorVisible) || (!visible && view->cursorVisible))
            _drawCursor(view);
    }

    view->cursorVisible = visible;
}

void UI_moveCursor (UI_view view, uint16_t row, uint16_t col)
{
    if (view->cursorVisible && view->visible && (view==g_active_view) )
        _drawCursor(view);
    view->cursorRow = row;
    view->cursorCol = col;
    if (view->cursorVisible && view->visible && (view==g_active_view) )
        _drawCursor(view);
}

void UI_getCursorPos (UI_view view, uint16_t *row, uint16_t *col)
{
    *row = view->cursorRow;
    *col = view->cursorCol;
}

void UI_activateView (UI_view view)
{
    if (g_active_view && g_active_view->cursorVisible && g_active_view->visible)
        _drawCursor(g_active_view);

    g_active_view = view;

    if (g_active_view && g_active_view->cursorVisible && g_active_view->visible)
        _drawCursor(g_active_view);
}

void UI_setViewVisible (UI_view view, bool visible)
{
    view->visible = visible;
    _updateLayout();
}

bool UI_isViewVisible (UI_view view)
{
    return view->visible;
}

void UI_clearView (UI_view view)
{
    if (!view->visible)
        return;

    SetAPen(g_rp, g_themes[g_theme].bg[0]);
    SetBPen(g_rp, g_themes[g_theme].bg[0]);
    SetDrMd(g_rp, JAM1);
    RectFill (g_rp, view->x, view->y, view->x + view->w - 1, view->y + view->h - 1);
}

void UI_scrollUp (UI_view view)
{
    if (!view->visible)
        return;

    if (view->cursorVisible && (view==g_active_view) )
        _drawCursor(view);
    WORD min_x = view->x;
    WORD min_y = view->y;
    WORD max_x = view->x+view->w-1;
    WORD max_y = view->y+view->h-1; // FIXME fullscreen ? g_win->Height-g_OffBottom-1 : g_OffTop + g_scrollEnd*g_fontHeight-1;
    LOG_printf (LOG_DEBUG, "UI: view: view->x=%d, view->y=%d, view->w=%d, view->h=%d\n", view->x, view->y, view->w, view->h);
    LOG_printf (LOG_DEBUG, "UI: scrollUp (%d/%d)-(%d/%d)\n", min_x, min_y, max_x, max_y);
#if 0
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


#ifdef ENABLE_SCROLL_BENCHMARK
    float startTime = U_getTime();
#endif
    ScrollRaster(g_rp, 0, g_fontHeight, min_x, min_y, max_x, max_y);
#ifdef ENABLE_SCROLL_BENCHMARK
    float stopTime = U_getTime();
    LOG_printf (LOG_DEBUG, "UI: ScrollRaster [UP] took %d-%d = %d ms\n", (int)stopTime, (int)startTime, (int) (1000.0 * (stopTime-startTime)));
#endif

#endif
    if (view->cursorVisible && (view==g_active_view) )
        _drawCursor(view);
}

void UI_scrollDown (UI_view view)
{
    if (!view->visible)
        return;

    if (view->cursorVisible && (view==g_active_view) )
        _drawCursor(view);

    WORD min_x = view->x;
    WORD min_y = view->y;
    WORD max_x = view->x+view->w-1;
    WORD max_y = view->y+view->h-1; // FIXME: + g_scrollEnd*g_fontHeight-1;

    LOG_printf (LOG_DEBUG, "UI: view: view->x=%d, view->y=%d, view->w=%d, view->h=%d\n", view->x, view->y, view->w, view->h);
    LOG_printf (LOG_DEBUG, "UI: scrollDown (%d/%d)-(%d/%d)\n", min_x, min_y, max_x, max_y);

#ifdef ENABLE_SCROLL_BENCHMARK
    float startTime = U_getTime();
#endif
    ScrollRaster(g_rp, 0, -g_fontHeight, min_x, min_y, max_x, max_y);
#ifdef ENABLE_SCROLL_BENCHMARK
    float stopTime = U_getTime();
    LOG_printf (LOG_DEBUG, "IDE: ScrollRaster [DOWN] took %d-%d = %d ms\n", (int)stopTime, (int)startTime, (int) (1000.0 * (stopTime-startTime)));
#endif

    if (view->cursorVisible && (view==g_active_view) )
        _drawCursor(view);
}

void UI_onEventCall (UI_view view, UI_event_cb cb, void *user_data)
{
    view->event_cb = cb;
    view->event_cb_user_data = user_data;
}

uint16_t UI_EZRequest (char *body, char *gadgets, ...)
{
    char *posTxt=NULL;
    char *negTxt=NULL;

    static char buf[256];
    strncpy (buf, gadgets, 256);
    char *s = buf;
    char *c = buf;
    while (*c)
    {
        if (*c=='|')
        {
            *c = 0;
            if (negTxt)
                posTxt = negTxt;
            negTxt = s;
            c++;
            s=c;
        }
        else
        {
            c++;
        }
    }
    *c = 0;
    if (negTxt)
        posTxt = negTxt;
    negTxt = s;

    va_list args;
    va_start(args, gadgets);
    static char buf2[1024];
    vsnprintf (buf2, 1024, body, args);
    va_end(args);
    bool b = U_request (g_win, posTxt, negTxt, "%s", buf2);

    return b ? 1 : 0;
}

char *UI_FileReq (char *title)
{
	static char pathbuf[1024];

    if (AslRequest(g_ASLFileReq, 0L))
    {
        //printf("PATH=%s FILE=%s\n", fr->rf_Dir, fr->rf_File);
        strncpy (pathbuf, (char*)g_ASLFileReq->rf_Dir, 1024);
        AddPart ((STRPTR) pathbuf, g_ASLFileReq->rf_File, 1024);
        //printf(" -> %s\n", pathbuf);

        return String (UP_ide, pathbuf);
    }

	return NULL;
}

#define FR_GID_FIND   1
#define FR_GID_CANCEL 2
#define FR_GID_STRING 3

bool UI_FindReq (char *buf, uint16_t buf_len, bool *matchCase, bool *wholeWord, bool *searchBackwards)
{

    struct Window   *reqwin = NULL;
    bool             asleep = FALSE;
    struct Gadget   *glist  = NULL;
    struct Gadget   *gad    = NULL;
    struct Gadget   *sgad=NULL, *caseGad=NULL, *wordGad=NULL, *backwardsGad=NULL;
    bool             res    = FALSE;

    if (! (gad = CreateContext(&glist)) )
        goto closedown;

    LOG_printf (LOG_DEBUG, "UI_FindReq buf: 0x%08lx (%s)\n", buf, buf);
    if ( !(sgad = gad = _createGadgetTags (STRING_KIND, gad, 55, 5, 327, 14, "Find:", FR_GID_STRING, 0, GTST_String, (ULONG) buf, GTST_MaxChars, buf_len, TAG_END)) )
        goto closedown;

    if ( !(caseGad = gad = _createGadgetTags (CHECKBOX_KIND, gad, 56, 25, 23, 14, "Match Case", 0, PLACETEXT_RIGHT, GTCB_Checked, *matchCase, TAG_END)) )
        goto closedown;

    if ( !(wordGad = gad = _createGadgetTags (CHECKBOX_KIND, gad, 56, 41, 23, 14, "Whole Word", 0, PLACETEXT_RIGHT, GTCB_Checked, *wholeWord, TAG_END)) )
        goto closedown;

    if ( !(backwardsGad = gad = _createGadgetTags (CHECKBOX_KIND, gad, 215, 25, 23, 14, "Backwards", 0, PLACETEXT_RIGHT, GTCB_Checked, *searchBackwards, TAG_END)) )
        goto closedown;

    if ( !(gad = _createGadgetTags (BUTTON_KIND, gad, 27, 58, 112, 12, "Find", FR_GID_FIND, 0, TAG_END)) )
        goto closedown;

    if ( !(gad = _createGadgetTags (BUTTON_KIND, gad, 248, 58, 112, 12, "Cancel", FR_GID_CANCEL, 0, TAG_END)) )
        goto closedown;

    if (!(reqwin = OpenWindowTags(  NULL,
                                    WA_Left,        g_win->LeftEdge + g_win->Width/2 - 200,
                                    WA_Top,         g_win->TopEdge + g_win->Height/2 - 45,
                                    WA_Width,       400,
                                    WA_Height,      90,
                                    WA_IDCMP,       IDCMP_RAWKEY | IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
                                    // FIXME WA_Flags,       ,
                                    // FIXME WA_AutoAdjust,  TRUE,
                                    // FIXME WA_CustomScreen,screen,
                                    WA_Title,         (ULONG) "Find...",
                                    WA_SmartRefresh,  TRUE,
                                    WA_Activate,      TRUE,
                                    WA_Borderless,    FALSE,
                                    WA_SizeGadget,    FALSE,
                                    WA_DragBar,       TRUE,
                                    WA_DepthGadget,   TRUE,
                                    WA_CloseGadget,   TRUE,
                                    WA_Gadgets,       (ULONG) glist,
                                    TAG_DONE)) )
        goto closedown;

    // block main window via a sleep requester

    struct Requester sleepReq;

    InitRequester(&sleepReq);
    asleep = Request(&sleepReq, g_win);

    GT_RefreshWindow (reqwin, NULL);
    ActivateGadget (sgad, reqwin, NULL);

    bool looping = TRUE;
    while (looping)
    {
        struct IntuiMessage *msg;

        if (!(msg = GT_GetIMsg(reqwin->UserPort)))
        {
            Wait (1L<<reqwin->UserPort->mp_SigBit);
            continue;
        }

        ULONG class = msg->Class;
        UWORD code = msg->Code;
        struct Gadget *gad = (struct Gadget *)msg->IAddress;
        GT_ReplyIMsg(msg);

        LOG_printf (LOG_DEBUG, "UI_FindReq got GT intui message, class=%d\n", class);

        switch (class)
        {
            case IDCMP_GADGETUP:
                switch (gad->GadgetID)
                {
                    case FR_GID_FIND:
                        looping = FALSE;
                        res = TRUE;
                        break;
                    case FR_GID_CANCEL:
                        looping = FALSE;
                        break;
                }
                break;
            case IDCMP_CLOSEWINDOW:
                looping = FALSE;
                break;
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(reqwin);
                GT_EndRefresh(reqwin, TRUE);
                break;
            case IDCMP_RAWKEY:
                LOG_printf (LOG_DEBUG, "IDCMP_RAWKEY code=%d\n", code);
                switch (code)
                {
                    case 0x44:	// enter
                        looping = FALSE;
                        res = TRUE;
                        break;
                    case 0x45:	// escape
                        looping = FALSE;
                        break;
                }
                break;
        }
    }

    strncpy (buf, (char *) ((struct StringInfo *)sgad->SpecialInfo)->Buffer, buf_len);
    *matchCase = caseGad->Flags & GFLG_SELECTED;
    *wholeWord = wordGad->Flags & GFLG_SELECTED;
    *searchBackwards = backwardsGad->Flags & GFLG_SELECTED;

closedown:

    if (asleep)
        EndRequest (&sleepReq, g_win);

    if (reqwin)
        CloseWindow(reqwin);

    if (glist)
        FreeGadgets (glist);

    return res;
}

void UI_HelpBrowser (void)
{
    static char pathbuf[256];
    strncpy (pathbuf, aqb_home, 256);
    AddPart ((STRPTR) pathbuf, (STRPTR)"README.guide", 256);

    DEBUG_help ("SYS:Utilities/MultiView", pathbuf);
}

struct MsgPort *UI_debugPort(void)
{
    return g_debugPort;
}

void UI_deinit(void)
{
	// FIXME: free find requester

    if (g_ASLFileReq)
		FreeAslRequest(g_ASLFileReq);

    for (int i=0; i<NUM_VIEWS; i++)
    {
        UI_view view = g_views[i];
        if (!view)
            continue;

        if (view->renderBM)
        {
            for (uint8_t planeNum = 0; planeNum < view->renderBM->Depth; planeNum++)
            {
                if (view->renderBM->Planes[planeNum])
                    FreeRaster(view->renderBM->Planes[planeNum], view->visWidth, BM_HEIGHT);
            }
            FreeMem(view->renderBM, sizeof(struct BitMap));
        }
        g_views[i] = NULL;
    }

    if (g_win)
	{
		ClearMenuStrip(g_win);
		if (g_menuStrip)
			FreeMenus (g_menuStrip);
        CloseWindow(g_win);
	}
    FreeGadgets(g_glist);
	if (g_vi)
		FreeVisualInfo(g_vi);
    if (g_debugPort)
        ASUP_delete_port(g_debugPort);
    if (ConsoleDevice)
        CloseDevice((struct IORequest *)&console_ioreq);
}

bool UI_init (void)
{
    LOG_printf (LOG_DEBUG, "UI_init starts.\n");

    SysBase = *(APTR *)4L;

    // check library versions

    if ( ((struct Library *)IntuitionBase)->lib_Version < 37)
         cleanexit("intuition library is too old, need at least V37", RETURN_FAIL);
    if ( ((struct Library *)GfxBase)->lib_Version < 37)
         cleanexit("graphics library is too old, need at least V37", RETURN_FAIL);
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

	WORD visWidth  = sc->Width > maxW ? maxW : sc->Width;
	WORD visHeight = sc->Height > maxH ? maxH : sc->Height;

    static char winTitle[128];
    snprintf (winTitle, 128, "%s %s", PROGRAM_NAME_LONG, VERSION);

    // open a full screen window
    if (!(g_win = OpenWindowTags(NULL,
                                 WA_Top,           sc->BarHeight+1,
                                 WA_Width,         visWidth,
                                 WA_Height,        visHeight-sc->BarHeight-1,
                                 WA_IDCMP,         IDCMP_MENUPICK | IDCMP_RAWKEY | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_GADGETUP | IDCMP_REQCLEAR,
                                 WA_Backdrop,      FALSE,
                                 WA_SmartRefresh,  TRUE,
                                 WA_Activate,      TRUE,
                                 WA_Borderless,    FALSE,
                                 WA_SizeGadget,    TRUE,
                                 WA_DragBar,       TRUE,
                                 WA_DepthGadget,   TRUE,
                                 WA_CloseGadget,   TRUE,
                                 WA_Title,         (LONG)winTitle,
                                 WA_MinWidth,      240,
                                 WA_MinHeight,     100,
                                 WA_MaxWidth,      visWidth,
                                 WA_MaxHeight,     visHeight,
                                 WA_NewLookMenus,  TRUE)))
         cleanexit("Can't open window", RETURN_FAIL);

    LOG_printf (LOG_DEBUG, "UI_init window opened.\n");

    UI_setColorScheme(OPT_prefGetInt (OPT_PREF_COLORSCHEME));

    /*
     * gadgets
     */

    if (!(g_vi = GetVisualInfo(g_win->WScreen, TAG_END)))
		cleanexit ("failed to get screen visual info", RETURN_FAIL);
    if (! (g_gad = CreateContext (&g_glist)))
        cleanexit("failed to create gadtools context", RETURN_FAIL);

    // create views
    UBYTE depth = sc->BitMap.Depth > 3 ? 3 : sc->BitMap.Depth;

    UnlockPubScreen(NULL, sc);

    g_views[UI_viewEditor]    = UI_View(depth, visWidth, /* scrollable=*/ FALSE);
    g_views[UI_viewStatusBar] = UI_View(depth, visWidth, /* scrollable=*/ FALSE);
    g_views[UI_viewConsole]   = UI_View(depth, visWidth, /* scrollable=*/ TRUE);

    UI_setViewVisible(g_views[UI_viewConsole], FALSE);

    _updateLayout();

    g_rp = g_win->RPort;

    UI_setFont(OPT_prefGetInt (OPT_PREF_FONT));

    LOG_printf (LOG_DEBUG, "UI_init views created.\n");

	/*
     * menu
     */

	if (!(g_menuStrip = CreateMenus(g_newmenu, TAG_END)))
		cleanexit("failed to create menu", RETURN_FAIL);
	if (!LayoutMenus(g_menuStrip, g_vi, GTMN_NewLookMenus, TRUE, TAG_END))
		cleanexit("failed to layout menu", RETURN_FAIL);
	if (!SetMenuStrip(g_win, g_menuStrip))
		cleanexit("failed to set menu strip", RETURN_FAIL);

    struct MenuItem *item = ItemAddress(g_menuStrip, FULLMENUNUM(/*menu=*/4, /*item=*/0, /*sub=*/OPT_prefGetInt (OPT_PREF_COLORSCHEME)));
    item->Flags |= CHECKED;
    item = ItemAddress(g_menuStrip, FULLMENUNUM(/*menu=*/4, /*item=*/1, /*sub=*/OPT_prefGetInt (OPT_PREF_FONT)));
    item->Flags |= CHECKED;

    /*
     * debug port
     */

    g_debugPort  = ASUP_create_port ((STRPTR) "AQB debug reply port", 0);
    if (!g_debugPort)
		cleanexit("failed to create debug port", RETURN_FAIL);

    /*
     * ASL file requester
     */

	if (! (g_ASLFileReq = (struct FileRequester *) AllocAslRequestTags(ASL_FileRequest,
			                                                           ASL_Hail,      (ULONG)"AQB",
			                                                           ASL_Dir,       (ULONG)aqb_home,
			                                                           ASL_File,      (ULONG)"",
														               ASL_Pattern,   (ULONG)"#?.bas",
														               ASL_FuncFlags, FILF_PATGAD,
			                                                           ASL_Window,    (ULONG) g_win,
			                                                           TAG_DONE)) )
		cleanexit("failed to allocate ASL file requester", RETURN_FAIL);

    LOG_printf (LOG_DEBUG, "UI_init complete.\n");
	return TRUE;
}

void UI_run (void)
{
    BOOL running = TRUE;
    while (running)
    {
        uint16_t key = nextEvent();
        if (key == KEY_QUIT)
            running = FALSE;
        else
        {
            if (key == KEY_NONE)
                continue;
            if (!g_active_view)
                continue;
            if (g_active_view->event_cb)
                g_active_view->event_cb (g_active_view, key, g_active_view->event_cb_user_data);
        }
	}
}

#endif

