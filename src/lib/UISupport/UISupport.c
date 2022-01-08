#include "../_aqb/_aqb.h"
#include "../_brt/_brt.h"

#include "UISupport.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <inline/intuition.h>

#include <clib/gadtools_protos.h>
#include <inline/gadtools.h>
#include <libraries/gadtools.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

struct Library           *GadToolsBase  = NULL;

static GTGADGET_t        *g_gtgadget_first = NULL;
static GTGADGET_t        *g_gtgadget_last  = NULL;

typedef struct
{
    struct Gadget     *gad;
    struct Gadget     *gadList;
    struct VisualInfo *vinfo;
    struct TextAttr    ta;
    BOOL               close_cb_installed;
    BOOL               msg_cb_installed;
    BOOL               deployed;
} ui_win_ext_t;

static ui_win_ext_t    g_win_ext[MAX_NUM_WINDOWS];

static BOOL window_msg_cb (SHORT wid, struct Window *win, struct IntuiMessage *message)
{
    DPRINTF ("UISupport: window_msg_cb called\n");

    ULONG class = message->Class;
    switch (class)
    {
        case IDCMP_REFRESHWINDOW:
            GT_BeginRefresh (win);
            GT_EndRefresh (win, TRUE);
            return TRUE;    // handled.
        case IDCMP_GADGETUP:
        {
            struct Gadget *g = (struct Gadget *) message->IAddress;

            if (!g)
                return FALSE;

            GTGADGET_t *gtg = (GTGADGET_t *) g->UserData;
            if (!gtg)
                return FALSE;

            if (gtg->gadgetup_cb)
                gtg->gadgetup_cb (wid, gtg->id, gtg);

            return TRUE;    // handled.
        }
    }

    return FALSE; // not handled -> std msg handling will take over
}

static void _gtgadgets_free (struct Window *win, ui_win_ext_t *ext)
{
    if (ext->deployed)
    {
		DPRINTF ("GTGADGETS_FREE: was deployed\n");
        RemoveGList (_g_cur_win, ext->gadList, -1);
        ext->deployed = FALSE;
    }

    if (ext->gadList)
    {
		DPRINTF ("GTGADGETS_FREE: g_gadList not null\n");
        FreeGadgets (ext->gadList);
        ext->gadList = NULL;
    }
}

static void window_close_cb (short win_id)
{
    DPRINTF ("UISupport: window_close_cb called on win #%d\n", win_id);

    ui_win_ext_t *ext = &g_win_ext[win_id];
    _gtgadgets_free (_g_winlist[win_id], ext);
}

#define _createGadgetTags(kind, prev, x, y, w, h, txt, user_data, flags, ta, vinfo, ___tagList, ...) \
    ({_sfdc_vararg _tags[] = { ___tagList, __VA_ARGS__ }; _createGadgetTagList((kind), (prev), (x), (y), (w), (h), (txt), (user_data), (flags), (ta), (vinfo), (const struct TagItem *) _tags); })


static struct Gadget *_createGadgetTagList (ULONG kind, struct Gadget *prev, WORD x, WORD y, WORD w, WORD h,
                                            char *txt, APTR user_data, ULONG flags, struct TextAttr *ta, struct VisualInfo *vinfo,
                                            const struct TagItem *tags)
{
    //_sfdc_vararg tags[] = { ___tagList, __VA_ARGS__ };
    //OpenScreenTagList((___newScreen), (const struct TagItem *) _tags); }

    struct NewGadget ng;

    ng.ng_LeftEdge   = x;
    ng.ng_TopEdge    = y;
    ng.ng_Width      = w;
    ng.ng_Height     = h;
    ng.ng_TextAttr   = ta;
    ng.ng_VisualInfo = vinfo;
    ng.ng_GadgetText = (STRPTR) txt;
    ng.ng_GadgetID   = 0;
    ng.ng_Flags      = flags;
    ng.ng_UserData   = user_data;

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

GTGADGET_t *GTGADGET_ (SHORT kind,
                       BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                       char *txt, ULONG flags, SHORT id, ...)
{

    DPRINTF ("GTGADGET_: creating a gadtools gadget of kind %d %d/%d - %d/%d\n", kind, x1, y1, x2, y2);
    //Delay (50);

    _aqb_get_output (/*needGfx=*/TRUE);

    ui_win_ext_t *ext = &g_win_ext[_g_cur_win_id];

    if (!ext->gadList)
    {
        ext->gad = CreateContext (&ext->gadList);
        if (!ext->gad)
        {
            DPRINTF ("GTGADGET_: CreateContext() failed.\n");
            ERROR(AE_GTGADGET_CREATE);
            return NULL;
        }
    }
	if (!ext->close_cb_installed)
	{
		DPRINTF ("GTGADGET_: installing custom close callback for the current window\n");
		_window_add_close_cb (window_close_cb);
		ext->close_cb_installed = TRUE;
	}

    DPRINTF ("GTGADGET_: #0\n");
    //Delay (50);

    if (!ext->vinfo)
    {
        ext->vinfo = GetVisualInfo(_g_cur_win->WScreen, TAG_END);
        if (!ext->vinfo)
        {
            DPRINTF ("GTGADGET_: GetVisualInfo() failed.\n");
            ERROR(AE_GTGADGET_CREATE);
            return NULL;
        }

        ext->ta.ta_Name  = (STRPTR) _g_cur_rp->Font->tf_Message.mn_Node.ln_Name;
        ext->ta.ta_YSize = _g_cur_rp->Font->tf_YSize;
        ext->ta.ta_Style = _g_cur_rp->Font->tf_Style;
        ext->ta.ta_Flags = _g_cur_rp->Font->tf_Flags;
    }

    GTGADGET_t *gtgadget = AllocVec(sizeof(*gtgadget), MEMF_CLEAR);
    if (!gtgadget)
    {
        ERROR(AE_GTGADGET_CREATE);
        return NULL;
    }

    gtgadget->prev = g_gtgadget_last;
    if (g_gtgadget_last)
        g_gtgadget_last = g_gtgadget_last->next = gtgadget;
    else
        g_gtgadget_first = g_gtgadget_last = gtgadget;

    gtgadget->id = id;

    DPRINTF ("GTGADGET_: #1\n");
    //Delay (50);

	ext->gad = _createGadgetTags (kind, ext->gad, x1, y1, x2-x1+1, y2-y1+1, txt, gtgadget, flags, &ext->ta, ext->vinfo,  TAG_END);

	if (!ext->gad)
	{
		DPRINTF ("GTGADGET_: CreateGadget() failed.\n");
		ERROR(AE_GTGADGET_CREATE);
		return NULL;
	}

    DPRINTF ("GTGADGET_: CreateGadget worked, gtgadget->gad=0x%08lx\n", ext->gad);
    //Delay (50);

    if (!ext->msg_cb_installed)
    {
        DPRINTF ("GTGADGET_: installing custom msg callback for the current window\n");
        _window_add_msg_cb (window_msg_cb);
        ext->msg_cb_installed = TRUE;
    }

    // take care of IDCMP flags
    ULONG gidcmp = 0;
    switch (kind)
    {
        case BUTTON_KIND  : gidcmp = BUTTONIDCMP  ; break;
        case CHECKBOX_KIND: gidcmp = CHECKBOXIDCMP; break;
        case INTEGER_KIND : gidcmp = INTEGERIDCMP ; break;
        case LISTVIEW_KIND: gidcmp = LISTVIEWIDCMP; break;
        case MX_KIND      : gidcmp = MXIDCMP      ; break;
        case NUMBER_KIND  : gidcmp = NUMBERIDCMP  ; break;
        case CYCLE_KIND   : gidcmp = CYCLEIDCMP   ; break;
        case PALETTE_KIND : gidcmp = PALETTEIDCMP ; break;
        case SCROLLER_KIND: gidcmp = SCROLLERIDCMP; break;
        case SLIDER_KIND  : gidcmp = SLIDERIDCMP  ; break;
        case STRING_KIND  : gidcmp = STRINGIDCMP  ; break;
        case TEXT_KIND    : gidcmp = TEXTIDCMP    ; break;
    }

	DPRINTF("GTGADGET_: _g_cur_win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", _g_cur_win->IDCMPFlags, gidcmp);

	if (gidcmp && ( (_g_cur_win->IDCMPFlags & gidcmp) != gidcmp ) )
		ModifyIDCMP (_g_cur_win, _g_cur_win->IDCMPFlags | gidcmp);

    //Delay (50);

    return gtgadget;
}

void GTGADGETS_DEPLOY (void)
{
    DPRINTF ("GADGETS_DEPLOY called\n");

    _aqb_get_output (/*needGfx=*/TRUE);

    ui_win_ext_t *ext = &g_win_ext[_g_cur_win_id];

    if (!ext->gadList)
    {
		DPRINTF ("GTGADGETS_DEPLOY: g_gadList is NULL for this window\n");
		ERROR(AE_GTGADGET_DEPLOY);
		return;
    }

    if (ext->deployed)
    {
		DPRINTF ("GTGADGETS_DEPLOY: already deployed\n");
		ERROR(AE_GTGADGET_DEPLOY);
		return;
    }

    AddGList (_g_cur_win, ext->gadList, 1000, -1, NULL);
    GT_RefreshWindow (_g_cur_win, NULL);
    RefreshGadgets (ext->gadList, _g_cur_win, NULL);
    GT_BeginRefresh (_g_cur_win);
    GT_EndRefresh (_g_cur_win, TRUE);

    ext->deployed = TRUE;
}

void GTGADGETS_FREE (void)
{
    DPRINTF ("GADGETS_FREE called\n");

    _aqb_get_output (/*needGfx=*/TRUE);

    ui_win_ext_t *ext = &g_win_ext[_g_cur_win_id];

    _gtgadgets_free (_g_cur_win, ext);
}

void ON_GTGADGETUP_CALL (GTGADGET_t *g, gtgadgetup_cb_t cb)
{
    g->gadgetup_cb = cb;
}

static void _UISupport_shutdown(void)
{
    DPRINTF ("_UISupport_shutdown called\n");

    GTGADGET_t *g = g_gtgadget_first;
    while (g)
    {
        GTGADGET_t *ng = g->next;
        FreeVec(g);
        g = ng;
    }

    for (int i=0; i<MAX_NUM_WINDOWS; i++)
    {
        DPRINTF ("_UISupport_shutdown window #%d\n", i);
        ui_win_ext_t *ext = &g_win_ext[i];
        if (ext->vinfo)
        {
            DPRINTF ("_UISupport_shutdown FreeVisualInfo()\n");
            FreeVisualInfo (ext->vinfo);
        }
    }

    if (GadToolsBase)
    {
        DPRINTF ("_UISupport_shutdown CloseLibrary (GadToolsBase)\n");
        CloseLibrary (GadToolsBase);
    }

    DPRINTF ("_UISupport_shutdown done\n");
}

void _UISupport_init(void)
{
    if (!(GadToolsBase = OpenLibrary((CONST_STRPTR) "gadtools.library", 0)))
        _cshutdown(20, (UBYTE *)"*** error: failed to open gadtools.library!\n");

    for (int i=0; i<MAX_NUM_WINDOWS; i++)
    {
        ui_win_ext_t *ext = &g_win_ext[i];

        ext->gad                = NULL;
        ext->gadList            = NULL;
        ext->vinfo              = NULL;
        ext->msg_cb_installed   = FALSE;
        ext->close_cb_installed = FALSE;
        ext->deployed           = FALSE;
    }

    ON_EXIT_CALL(_UISupport_shutdown);
}

