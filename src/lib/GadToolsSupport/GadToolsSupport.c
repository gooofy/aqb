#include "../_aqb/_aqb.h"
#include "../_brt/_brt.h"

#include "GadToolsSupport.h"

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

extern struct Library    *GadToolsBase ;

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
    BOOL handled = FALSE;

    struct IntuiMessage *imsg = GT_FilterIMsg(message);

    if (imsg)
    {
        ULONG class = imsg->Class;

        DPRINTF ("GadToolsSupport: window_msg_cb class=0x%08lx\n", class);

        switch (class)
        {
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh (win);
                GT_EndRefresh (win, TRUE);
                handled = TRUE;
                break;

            case IDCMP_GADGETUP:
            {
                struct Gadget *g = (struct Gadget *) imsg->IAddress;

                if (!g)
                    break;

                GTGADGET_t *gtg = (GTGADGET_t *) g->UserData;
                if (!gtg)
                    break;

                if (gtg->gadgetup_cb)
                    gtg->gadgetup_cb (wid, gtg->id, imsg->Code, gtg->gadgetup_user_data);

                handled = TRUE;
                break;
            }

            case IDCMP_GADGETDOWN:
            {
                struct Gadget *g = (struct Gadget *) imsg->IAddress;

                if (!g)
                    break;

                GTGADGET_t *gtg = (GTGADGET_t *) g->UserData;
                if (!gtg)
                    break;

                if (gtg->gadgetdown_cb)
                    gtg->gadgetdown_cb (wid, gtg->id, imsg->Code, gtg->gadgetdown_user_data);

                handled = TRUE;
                break;
            }

            case IDCMP_MOUSEMOVE:
            {
                struct Gadget *g = (struct Gadget *) imsg->IAddress;

                if (!g)
                    break;

                GTGADGET_t *gtg = (GTGADGET_t *) g->UserData;
                if (!gtg)
                    break;

                if (gtg->gadgetmove_cb)
                    gtg->gadgetmove_cb (wid, gtg->id, imsg->Code, gtg->gadgetmove_user_data);

                handled = TRUE;
                break;
            }
        }
    }

    GT_PostFilterIMsg (imsg);

    return handled;
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
    DPRINTF ("GadToolsSupport: window_close_cb called on win #%d\n", win_id);

    ui_win_ext_t *ext = &g_win_ext[win_id];
    struct Window *win = _aqb_get_win(win_id);
    _gtgadgets_free (win, ext);
}

#define _createGadgetTags(kind, prev, x, y, w, h, txt, user_data, flags, ta, vinfo, ___tagList, ...) \
    ({_sfdc_vararg _tags[] = { ___tagList, __VA_ARGS__ }; _createGadgetTagList((kind), (prev), (x), (y), (w), (h), (txt), (user_data), (flags), (ta), (vinfo), (const struct TagItem *) _tags); })


static struct Gadget *_createGadgetTagList (ULONG kind, struct Gadget *prev, WORD x, WORD y, WORD w, WORD h,
                                            char *txt, APTR user_data, ULONG flags, struct TextAttr *ta, struct VisualInfo *vinfo,
                                            const struct TagItem *tags)
{

    DPRINTF ("_createGadgetTagList: flags=0x%08lx\n", flags);

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

    return CreateGadgetA (kind, prev, &ng, tags);
}

GTGADGET_t *GTGADGET_ (SHORT kind,
                       BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                       char *txt, ULONG flags, SHORT id, ULONG ti_Tag, ...)
{

    DPRINTF ("GTGADGET_: creating a gadtools gadget of kind %d %d/%d - %d/%d\n", kind, x1, y1, x2, y2);
    DPRINTF ("           txt=%s, flags=0x%08lx, id=%d\n", txt ? txt : "NULL", flags, id);

    _aqb_get_output (/*needGfx=*/TRUE);

    ui_win_ext_t *ext = &g_win_ext[_g_cur_win_id];

    if (!ext->gadList)
    {
        ext->gad = CreateContext (&ext->gadList);
        if (!ext->gad)
        {
            DPRINTF ("GTGADGET_: CreateContext() failed.\n");
            ERROR(AE_GTG_CREATE);
            return NULL;
        }
    }
	if (!ext->close_cb_installed)
	{
		DPRINTF ("GTGADGET_: installing custom close callback for the current window\n");
		_window_add_close_cb (window_close_cb);
		ext->close_cb_installed = TRUE;
	}

    if (!ext->vinfo)
    {
        ext->vinfo = GetVisualInfo(_g_cur_win->WScreen, TAG_END);
        if (!ext->vinfo)
        {
            DPRINTF ("GTGADGET_: GetVisualInfo() failed.\n");
            ERROR(AE_GTG_CREATE);
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
        ERROR(AE_GTG_CREATE);
        return NULL;
    }

    gtgadget->prev = g_gtgadget_last;
    if (g_gtgadget_last)
        g_gtgadget_last = g_gtgadget_last->next = gtgadget;
    else
        g_gtgadget_first = g_gtgadget_last = gtgadget;

    gtgadget->id  = id;
    gtgadget->win = _g_cur_win;

    va_list ap;
    va_start (ap, ti_Tag);
    struct TagItem *tags = _vatagitems (ti_Tag, ap);
    va_end(ap);

	gtgadget->gad = ext->gad = _createGadgetTagList (kind, ext->gad, x1, y1, x2-x1+1, y2-y1+1, txt, gtgadget, flags, &ext->ta, ext->vinfo, tags);

    DEALLOCATE (tags);

	if (!ext->gad)
	{
		DPRINTF ("GTGADGET_: CreateGadget() failed.\n");
		ERROR(AE_GTG_CREATE);
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

void GTG_MODIFY (GTGADGET_t *g, ULONG ti_Tag, ...)
{
    DPRINTF ("GTG_MODIFY called\n");

    if (!g || !g->gad)
    {
		DPRINTF ("GTG_MODIFY: invalid gadget\n");
		ERROR(AE_GTG_MODIFY);
		return;
    }

    va_list ap;
    va_start (ap, ti_Tag);
    struct TagItem *tags = _vatagitems (ti_Tag, ap);
    va_end(ap);

    GT_SetGadgetAttrsA (g->gad, g->win, /*req=*/NULL, tags);

    DEALLOCATE (tags);
}

BOOL GTGSELECTED_ (GTGADGET_t *g)
{
    DPRINTF ("GTGSELECTED_ called\n");

    if (!g || !g->gad)
    {
		DPRINTF ("GTGSELECTED_: invalid gadget\n");
		ERROR(AE_GTG_SELECTED);
		return FALSE;
    }

    return g->gad->Flags & GFLG_SELECTED;
}

STRPTR GTGBUFFER_ (GTGADGET_t *g)
{
    DPRINTF ("GTGBUFFER_ called\n");

    if (!g || !g->gad)
    {
		DPRINTF ("GTGBUFFER_: invalid gadget\n");
		ERROR(AE_GTG_BUFFER);
		return FALSE;
    }

    struct StringInfo * si = (struct StringInfo *)g->gad->SpecialInfo;

    return si->Buffer;
}

void GTGADGETS_DEPLOY (void)
{
    DPRINTF ("GADGETS_DEPLOY called\n");

    _aqb_get_output (/*needGfx=*/TRUE);

    ui_win_ext_t *ext = &g_win_ext[_g_cur_win_id];

    if (!ext->gadList)
    {
		DPRINTF ("GTGADGETS_DEPLOY: g_gadList is NULL for this window\n");
		ERROR(AE_GTG_DEPLOY);
		return;
    }

    if (ext->deployed)
    {
		DPRINTF ("GTGADGETS_DEPLOY: already deployed\n");
		ERROR(AE_GTG_DEPLOY);
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

void ON_GTG_UP_CALL (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data)
{
    if (!g || !g->gad)
    {
		DPRINTF ("ON_GTG_UP_CALL: invalid gadget\n");
		ERROR(AE_GTG_CALLBACK);
		return;
    }

    g->gadgetup_cb        = cb;
    g->gadgetup_user_data = user_data;
}

void ON_GTG_DOWN_CALL (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data)
{
    if (!g || !g->gad)
    {
		DPRINTF ("ON_GTG_DOWN_CALL: invalid gadget\n");
		ERROR(AE_GTG_CALLBACK);
		return;
    }

    g->gadgetdown_cb        = cb;
    g->gadgetdown_user_data = user_data;
}

void ON_GTG_MOVE_CALL (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data)
{
    if (!g || !g->gad)
    {
		DPRINTF ("ON_GTG_MOVE_CALL: invalid gadget\n");
		ERROR(AE_GTG_CALLBACK);
		return;
    }

    g->gadgetmove_cb        = cb;
    g->gadgetmove_user_data = user_data;
}

static void _GadToolsSupport_shutdown(void)
{
    DPRINTF ("_GadToolsSupport_shutdown called\n");

    GTGADGET_t *g = g_gtgadget_first;
    while (g)
    {
        GTGADGET_t *ng = g->next;
        FreeVec(g);
        g = ng;
    }

    for (int i=0; i<MAX_NUM_WINDOWS; i++)
    {
        DPRINTF ("_GadToolsSupport_shutdown window #%d\n", i);
        ui_win_ext_t *ext = &g_win_ext[i];
        if (ext->vinfo)
        {
            DPRINTF ("_GadToolsSupport_shutdown FreeVisualInfo()\n");
            FreeVisualInfo (ext->vinfo);
        }
    }

    DPRINTF ("_GadToolsSupport_shutdown done\n");
}

void _GadToolsSupport_init(void)
{
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

    ON_EXIT_CALL(_GadToolsSupport_shutdown);
}

