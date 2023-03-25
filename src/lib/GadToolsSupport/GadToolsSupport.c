#include "../_aqb/_aqb.h"
#include "../_brt/_brt.h"

#include "GadToolsSupport.h"

#include <exec/types.h>
//#include <exec/memory.h>
//#include <clib/exec_protos.h>
//#include <inline/exec.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <inline/intuition.h>

#include <clib/graphics_protos.h>
#include <inline/graphics.h>

#include <clib/gadtools_protos.h>
#include <inline/gadtools.h>

//#include <clib/dos_protos.h>
//#include <inline/dos.h>

extern struct Library    *GadToolsBase ;

gt_win_ext_t    _g_gt_win_ext[MAX_NUM_WINDOWS];

SHORT _GTGADGET_NEXT_ID (void)
{
    gt_win_ext_t *ext = &_g_gt_win_ext[_g_cur_win_id];
    return ext->id++;
}

void _GTGADGET_CONSTRUCTOR (GTGADGET_t *this, char *txt,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{

    DPRINTF("_GTGADGET_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, txt=%s\n", this, x1, y1, txt ? txt : "NULL");

    gt_win_ext_t *ext = &_g_gt_win_ext[_g_cur_win_id];

    if (ext->deployed)
    {
		DPRINTF ("_GTGADGET_CONSTRUCTOR: already deployed\n");
		ERROR(AE_GTG_CREATE);
		return;
    }

    this->next = NULL;
    this->prev = ext->last;
    if (ext->last)
        ext->last = ext->last->next = this;
    else
        ext->first = ext->last = this;

    this->ng.ng_LeftEdge   = x1;
    this->ng.ng_TopEdge    = y1;
    this->ng.ng_Width      = x2-x1+1;
    this->ng.ng_Height     = y2-y1+1;
    this->ng.ng_GadgetText = (STRPTR) txt;
    this->ng.ng_GadgetID   = _GTGADGET_NEXT_ID();
    this->ng.ng_Flags      = flags;
    this->ng.ng_UserData   = this;
    this->gad              = NULL;
    this->win              = _g_cur_win;
    this->user_data        = user_data;
    this->underscore       = underscore;
    this->gadgetup_cb      = NULL;
    this->gadgetdown_cb    = NULL;
    this->gadgetmove_cb    = NULL;
}

SHORT _GTGADGET_x1_ (GTGADGET_t *this)
{
    return this->ng.ng_LeftEdge;
}
void _GTGADGET_x1 (GTGADGET_t *this, SHORT x1)
{
    this->ng.ng_LeftEdge = x1;
}

SHORT _GTGADGET_y1_ (GTGADGET_t *this)
{
    return this->ng.ng_TopEdge;
}
void _GTGADGET_y1 (GTGADGET_t *this, SHORT y1)
{
    this->ng.ng_TopEdge = y1;
}

SHORT _GTGADGET_x2_ (GTGADGET_t *this)
{
    return this->ng.ng_LeftEdge + this->ng.ng_Width - 1;
}
void _GTGADGET_x2 (GTGADGET_t *this, SHORT x2)
{
    this->ng.ng_Width = x2-this->ng.ng_LeftEdge+1;
}

SHORT _GTGADGET_y2_ (GTGADGET_t *this)
{
    return this->ng.ng_TopEdge + this->ng.ng_Height - 1;
}
void _GTGADGET_y2 (GTGADGET_t *this, SHORT y2)
{
    this->ng.ng_Height = y2-this->ng.ng_TopEdge+1;
}

CONST_STRPTR _GTGADGET_text_ (GTGADGET_t *this)
{
    return this->ng.ng_GadgetText;
}
void _GTGADGET_text (GTGADGET_t *this, STRPTR text)
{
    DPRINTF ("_GTGADGET_text: text=%s\n", text ? (char*)text : "NULL");
    this->ng.ng_GadgetText = text;
}

SHORT  _GTGADGET_id_ (GTGADGET_t *this)
{
    return this->ng.ng_GadgetID;
}
void _GTGADGET_id (GTGADGET_t *this, SHORT id)
{
    this->ng.ng_GadgetID = id;
}

ULONG _GTGADGET_flags_ (GTGADGET_t *this)
{
    return this->ng.ng_Flags;
}
void _GTGADGET_flags (GTGADGET_t *this, ULONG flags)
{
    this->ng.ng_Flags = flags;
}

BOOL _GTGADGET_deployed_ (GTGADGET_t *this)
{
    return this->gad != NULL;
}


struct Gadget *_gtbutton_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTBUTTON_t *button = (GTBUTTON_t *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (BUTTON_KIND, gad, &gtg->ng, GA_Disabled, button->disabled, GT_Underscore, gtg->underscore, TAG_DONE);

	if (!gtg->gad)
	{
		DPRINTF ("_gtbutton_deploy_cb: CreateGadget() failed.\n");
		ERROR(AE_GTG_CREATE);
		return gad;
	}

    // take care of IDCMP flags
    ULONG gidcmp = BUTTONIDCMP;

	DPRINTF("_gtbutton_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

	if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
		ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _GTBUTTON_CONSTRUCTOR (GTBUTTON_t *this, char *txt,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTBUTTON_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d, txt=%s\n", this, x1, y1, x2, y2, txt ? txt : "NULL");
    _GTGADGET_CONSTRUCTOR (&this->gadget, txt, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtbutton_deploy_cb;
    this->disabled         = FALSE;
}

BOOL _GTBUTTON_disabled_ (GTBUTTON_t *this)
{
    return this->disabled;
}
void _GTBUTTON_disabled (GTBUTTON_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    this->disabled = disabled;
}

struct Gadget *_gtcheckbox_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTCHECKBOX_t *cb = (GTCHECKBOX_t *)gtg;

	DPRINTF("_gtcheckbox_deploy_cb: cb=0x%08lx, checked=%d\n", cb, cb->checked);

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (CHECKBOX_KIND, gad, &gtg->ng, GA_Disabled, cb->disabled, GT_Underscore, gtg->underscore, GTCB_Checked, cb->checked, TAG_DONE);

	if (!gtg->gad)
	{
		DPRINTF ("_gtcheckbox_deploy_cb: CreateGadget() failed.\n");
		ERROR(AE_GTG_CREATE);
		return gad;
	}

    // take care of IDCMP flags
    ULONG gidcmp = CHECKBOXIDCMP;

	DPRINTF("_gtcheckbox_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

	if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
		ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _GTCHECKBOX_CONSTRUCTOR (GTCHECKBOX_t *this, char *txt,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTCHECKBOX_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d, txt=%s\n", this, x1, y1, x2, y2, txt ? txt : "NULL");
    _GTGADGET_CONSTRUCTOR (&this->gadget, txt, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtcheckbox_deploy_cb;
    this->disabled         = FALSE;
    this->checked          = FALSE;
}

BOOL _GTCHECKBOX_disabled_ (GTCHECKBOX_t *this)
{
    return this->disabled;
}
void _GTCHECKBOX_disabled (GTCHECKBOX_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    this->disabled = disabled;
}

BOOL _GTCHECKBOX_checked_ (GTCHECKBOX_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget))
        return this->gadget.gad->Flags & GFLG_SELECTED;
    return this->checked;
}
void _GTCHECKBOX_checked (GTCHECKBOX_t *this, BOOL checked)
{
    DPRINTF ("_GTCHECKBOX_checked: this=0x%08x, checked=%d, size=%d\n", this, checked, sizeof (GTCHECKBOX_t));
    #if 1
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        DPRINTF ("_GTCHECKBOX_checked: is deployed, gad=0x%08x\n", this->gadget.gad);
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTCB_Checked, checked, TAG_DONE);
    }
    this->checked = checked;
    DPRINTF ("_GTCHECKBOX_checked: this=0x%08x, this->checked=%d\n", this, this->checked);
    #endif
}

#if 0
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

LONG GTGNUM_ (GTGADGET_t *g)
{
    DPRINTF ("GTGNUM_ called\n");

    if (!g || !g->gad)
    {
		DPRINTF ("GTGNUM_: invalid gadget\n");
		ERROR(AE_GTG_NUM);
		return FALSE;
    }

    struct StringInfo * si = (struct StringInfo *)g->gad->SpecialInfo;

    return si->LongInt;
}
#endif

static void _gtgadgets_free (struct Window *win, gt_win_ext_t *ext)
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

static void window_close_cb (short win_id, void *ud)
{
    DPRINTF ("GadToolsSupport: window_close_cb called on win #%d\n", win_id);

    gt_win_ext_t *ext = &_g_gt_win_ext[win_id];
    struct Window *win = _aqb_get_win(win_id);
    _gtgadgets_free (win, ext);
}

static BOOL window_msg_cb (SHORT wid, struct Window *win, struct IntuiMessage *message, window_refresh_cb_t refresh_cb, void *refresh_ud)
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
                if (refresh_cb)
                    refresh_cb(wid+1, refresh_ud);
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
                    gtg->gadgetup_cb (gtg, imsg->Code);

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
                    gtg->gadgetdown_cb (gtg, imsg->Code);

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
                    gtg->gadgetmove_cb (gtg, imsg->Code);

                handled = TRUE;
                break;
            }
        }
    }

    GT_PostFilterIMsg (imsg);

    return handled;
}

void GTGADGETS_DEPLOY (void)
{
    DPRINTF ("GADGETS_DEPLOY called\n");

    _aqb_get_output (/*needGfx=*/TRUE);

    gt_win_ext_t *ext = &_g_gt_win_ext[_g_cur_win_id];

    if (ext->deployed)
    {
		DPRINTF ("GTGADGETS_DEPLOY: already deployed -> redeploy\n");
        struct Window *win = _aqb_get_win(_g_cur_win_id);
        _gtgadgets_free (win, ext);
        Move (_g_cur_rp, 0, 0);
        ClearScreen(_g_cur_rp);
        ext->deployed = FALSE;
    }

	if (!ext->close_cb_installed)
	{
		DPRINTF ("GTGADGETS_DEPLOY: installing custom close callback for the current window\n");
		_window_add_close_cb (window_close_cb, NULL);
		ext->close_cb_installed = TRUE;
	}

    if (!ext->vinfo)
    {
        ext->vinfo = GetVisualInfo(_g_cur_win->WScreen, TAG_END);
        if (!ext->vinfo)
        {
            DPRINTF ("GTGADGETS_DEPLOY: GetVisualInfo() failed.\n");
            ERROR(AE_GTG_CREATE);
            return;
        }

        ext->ta.ta_Name  = (STRPTR) _g_cur_rp->Font->tf_Message.mn_Node.ln_Name;
        ext->ta.ta_YSize = _g_cur_rp->Font->tf_YSize;
        ext->ta.ta_Style = _g_cur_rp->Font->tf_Style;
        ext->ta.ta_Flags = _g_cur_rp->Font->tf_Flags;
    }

    ext->gad = CreateContext (&ext->gadList);
    if (!ext->gad)
    {
        DPRINTF ("GTGADGET_: CreateContext() failed.\n");
        ERROR(AE_GTG_CREATE);
        return;
    }

    GTGADGET_t *gtg = ext->first;
    while (gtg)
    {
        if (gtg->deploy_cb)
            ext->gad = gtg->deploy_cb(gtg, ext->gad, ext->vinfo, &ext->ta);
        gtg = gtg->next;
    }

    if (!ext->msg_cb_installed)
    {
        DPRINTF ("GTGADGET_: installing custom msg callback for the current window\n");
        _window_add_msg_cb (window_msg_cb);
        ext->msg_cb_installed = TRUE;
    }

    AddGList (_g_cur_win, ext->gadList, 1000, -1, NULL);
    GT_RefreshWindow (_g_cur_win, NULL);
    RefreshGadgets (ext->gadList, _g_cur_win, NULL);
    GT_BeginRefresh (_g_cur_win);
    GT_EndRefresh (_g_cur_win, TRUE);

    ext->deployed = TRUE;
}

#if 0
void GTGADGETS_FREE (void)
{
    DPRINTF ("GADGETS_FREE called\n");

    _aqb_get_output (/*needGfx=*/TRUE);

    gt_win_ext_t *ext = &_g_gt_win_ext[_g_cur_win_id];

    _gtgadgets_free (_g_cur_win, ext);
}

void GTG_DRAW_BEVEL_BOX (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, BOOL recessed )
{
    DPRINTF ("GTG_DRAW_BEVEL_BOX: %d/%d - %d/%d recessed=%d\n", x1, y1, x2, y2, recessed);

    _aqb_get_output (/*needGfx=*/TRUE);

    gt_win_ext_t *ext = &_g_gt_win_ext[_g_cur_win_id];
    if (recessed)
        DrawBevelBox (_g_cur_rp, x1, y1, x2-x1+1, y2-y1+1, GTBB_Recessed, TRUE, GT_VisualInfo, (ULONG) ext->vinfo, TAG_DONE);
    else
        DrawBevelBox (_g_cur_rp, x1, y1, x2-x1+1, y2-y1+1, GT_VisualInfo, (ULONG) ext->vinfo, TAG_DONE);
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
#endif

static void _GadToolsSupport_shutdown(void)
{
    DPRINTF ("_GadToolsSupport_shutdown called\n");

    for (int i=0; i<MAX_NUM_WINDOWS; i++)
    {
        DPRINTF ("_GadToolsSupport_shutdown window #%d\n", i);
        gt_win_ext_t *ext = &_g_gt_win_ext[i];
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
        gt_win_ext_t *ext = &_g_gt_win_ext[i];

        ext->first              = NULL;
        ext->last               = NULL;
        ext->gad                = NULL;
        ext->gadList            = NULL;
        ext->vinfo              = NULL;
        ext->msg_cb_installed   = FALSE;
        ext->close_cb_installed = FALSE;
        ext->deployed           = FALSE;
        ext->id                 = 1;
    }

    ON_EXIT_CALL(_GadToolsSupport_shutdown);
}

