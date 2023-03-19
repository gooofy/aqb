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

#include <clib/graphics_protos.h>
#include <inline/graphics.h>

#include <clib/gadtools_protos.h>
#include <inline/gadtools.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

extern struct Library    *GadToolsBase ;

typedef struct
{
    GTGADGET_t        *root;
    struct Gadget     *gad;
    struct Gadget     *gadList;
    APTR              *vinfo;
    struct TextAttr    ta;
    BOOL               close_cb_installed;
    BOOL               msg_cb_installed;
    BOOL               deployed;
} ui_win_ext_t;

static ui_win_ext_t    g_win_ext[MAX_NUM_WINDOWS];

void _GTGADGET_CONSTRUCTOR (GTGADGET_t *this, GTGADGET_t *parent,
                            char *txt, SHORT id,
                            void *user_data, ULONG flags, ULONG underscore)
{
    ui_win_ext_t *ext = &g_win_ext[_g_cur_win_id-1];

    DPRINTF("_GTGADGET_CONSTRUCTOR: this=0x%08lx, parent=0x%08lx, root=0x%08lx\n", this, parent, ext->root);

    if (ext->deployed)
    {
		DPRINTF ("_GTGADGET_CONSTRUCTOR: already deployed\n");
		ERROR(AE_GTG_CREATE);
		return;
    }

    if (ext->root)
    {
        if (!parent)
        {
            DPRINTF ("_GTGADGET_CONSTRUCTOR: already have a root gadget\n");
            ERROR(AE_GTG_CREATE);
            return;
        }
    }
    else
    {
        if (parent)
        {
            DPRINTF ("_GTGADGET_CONSTRUCTOR: need exactly one root gadget\n");
            ERROR(AE_GTG_CREATE);
            return;
        }
        ext->root = this;
    }

    this->gadgetup_cb      = NULL;
    this->gadgetdown_cb    = NULL;
    this->gadgetmove_cb    = NULL;
    this->user_data        = user_data;
    this->underscore       = underscore;
    this->next             = NULL;
    this->prev             = NULL;
    this->gad              = NULL;
    this->win_id           = _g_cur_win_id;
    this->deploy_cb        = NULL;
    this->add_child_cb     = NULL;
    this->domain_cb        = NULL;
    this->ng.ng_GadgetText = (STRPTR) txt;
    this->ng.ng_GadgetID   = id;
    this->ng.ng_Flags      = flags;
    this->ng.ng_UserData   = this;

    if (parent)
    {
        if (!parent->add_child_cb)
        {
            DPRINTF ("_GTGADGET_CONSTRUCTOR: parent is not a container gadget\n");
            ERROR(AE_GTG_CREATE);
            return;
        }
        parent->add_child_cb (parent, this);
    }
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

static void _GTLAYOUT_add_child_cb (GTGADGET_t *gtg, GTGADGET_t *child)
{
    GTLAYOUT_t *layout = (GTLAYOUT_t *)gtg;

    child->next = NULL;
    if (!layout->child_first)
    {
        layout->child_first = layout->child_last = child;
        child->prev = NULL;
    }
    else
    {
        child->prev = layout->child_last;
        layout->child_last = layout->child_last->next = child;
    }
}

static void _GTLAYOUT_calc_layout (GTLAYOUT_t *layout, SHORT which, SHORT *w, SHORT *h, SHORT *childc)
{
    *w=0, *h=0, *childc=0;

    if (layout->horiz)
    {
        GTGADGET_t *child = layout->child_first;
        while (child)
        {
            SHORT cw, ch;
            child->domain_cb (child, which, &cw, &ch);
            DPRINTF("_GTLAYOUT_calc_layout: child=0x%08lx -> nom dom cw=%d, ch=%d\n", child, cw, ch);
            if (ch > *h)
                *h = ch;
            *w += cw;
            *childc = *childc + 1;
            child = child->next;
        }

        DPRINTF("_GTLAYOUT_calc_layout: horizontal layout -> w=%d, h=%d\n", *w, *h);
    }
    else /* vertical layout */
    {
        GTGADGET_t *child = layout->child_first;
        while (child)
        {
            SHORT cw, ch;
            child->domain_cb (child, which, &cw, &ch);
            DPRINTF("_GTLAYOUT_calc_layout: child=0x%08lx -> nom dom cw=%d, ch=%d\n", child, cw, ch);
            if (cw > *w)
                *w = cw;
            *h += ch;
            *childc = *childc + 1;
            child = child->next;
        }

        DPRINTF("_GTLAYOUT_calc_layout: vertical layout -> w=%d, h=%d\n", *w, *h);
    }
}

static struct Gadget *_GTLAYOUT_apply_layout (GTLAYOUT_t *layout, struct Gadget *gad, APTR vinfo, struct TextAttr *ta,
                                              SHORT which, SHORT x, SHORT y, SHORT w, SHORT h, SHORT childw, SHORT childh, SHORT childc)
{
    if (childc)
    {
        if (layout->horiz)
        {
            SHORT curx=x;
            SHORT extraw = (w-childw)/childc;
            GTGADGET_t *child = layout->child_first;
            while (child)
            {
                SHORT cw, ch;
                child->domain_cb (child, GT_DOMAIN_NOMINAL, &cw, &ch);
                gad = child->deploy_cb (child, gad, vinfo, ta, curx, y, cw+extraw, h);
                curx += cw+extraw;
                child = child->next;
            }
        }
        else /* vertical layout */
        {
            SHORT cury=y;
            SHORT extrah = (h-childh)/childc;
            GTGADGET_t *child = layout->child_first;
            while (child)
            {
                SHORT cw, ch;
                child->domain_cb (child, GT_DOMAIN_NOMINAL, &cw, &ch);
                gad = child->deploy_cb (child, gad, vinfo, ta, x, cury, w, ch+extrah);
                cury += ch+extrah;
                child = child->next;
            }
        }
    }
    return gad;
}

static struct Gadget *_GTLAYOUT_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta,
                                           SHORT x, SHORT y, SHORT w, SHORT h)
{
    GTLAYOUT_t *layout = (GTLAYOUT_t *)gtg;
    DPRINTF("_GTLAYOUT_deploy_cb: gtg=0x%08lx, x=%d, y=%d, w=%d, h=%d\n", gtg, x, y, w, h);

    // try nominal sizes first

    SHORT childw=0, childh=0, childc=0;

    _GTLAYOUT_calc_layout (layout, GT_DOMAIN_NOMINAL, &childw, &childh, &childc);

    // FIXME: minimal layout in case nominal doesn't fit

    // apply layout

    return _GTLAYOUT_apply_layout(layout, gad, vinfo, ta, GT_DOMAIN_NOMINAL, x, y, w, h, childw, childh, childc);
}

static void _GTLAYOUT_domain_cb(GTGADGET_t *gtg, SHORT which, SHORT *w, SHORT *h)
{
    DPRINTF("_GTLAYOUT_domain_cb: gtg=0x%08lx, which=%d\n", gtg, which);

    GTLAYOUT_t *layout = (GTLAYOUT_t *)gtg;

    SHORT childc=0;

    _GTLAYOUT_calc_layout (layout, which, w, h, &childc);
}

void _GTLAYOUT_CONSTRUCTOR (GTLAYOUT_t *this, GTGADGET_t *parent, BOOL horiz)
{
    DPRINTF("_GTLAYOUT_CONSTRUCTOR: this=0x%08lx, parent=0x%08lx, horiz=%d\n", this, parent, horiz);

    if (!this)
    {
            DPRINTF ("_GTLAYOUT_CONSTRUCTOR: this==NULL\n");
            ERROR(AE_GTG_CREATE);
            return;
    }

    _GTGADGET_CONSTRUCTOR (&this->gadget, parent, /*txt=*/NULL, /*id=*/0, /*user_data=*/NULL, /*flags=*/0, /*underscore=*/0);
    this->horiz       = horiz;
    this->child_first = NULL;
    this->child_last  = NULL;

    this->gadget.add_child_cb = _GTLAYOUT_add_child_cb;
    this->gadget.domain_cb    = _GTLAYOUT_domain_cb;
    this->gadget.deploy_cb    = _GTLAYOUT_deploy_cb;
}

static struct Gadget *_GTBUTTON_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta,
                                           SHORT x, SHORT y, SHORT w, SHORT h)
{
    GTBUTTON_t *button = (GTBUTTON_t *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;
    gtg->ng.ng_LeftEdge   = x;
    gtg->ng.ng_TopEdge    = y;
    gtg->ng.ng_Width      = w;
    gtg->ng.ng_Height     = h;

    gtg->gad = CreateGadget (BUTTON_KIND, gad, &gtg->ng, GA_Disabled, button->disabled, GT_Underscore, gtg->underscore, TAG_DONE);

	if (!gtg->gad)
	{
		DPRINTF ("_gtbutton_deploy_cb: CreateGadget() failed.\n");
		ERROR(AE_GTG_CREATE);
		return gad;
	}

    // take care of IDCMP flags
    ULONG gidcmp = BUTTONIDCMP;

    struct Window *win = _aqb_get_win(gtg->win_id-1);
	DPRINTF("_gtbutton_deploy_cb: win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", win->IDCMPFlags, gidcmp);

	if (gidcmp && ( (win->IDCMPFlags & gidcmp) != gidcmp ) )
		ModifyIDCMP (win, win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

static void _GTBUTTON_domain_cb(GTGADGET_t *gtg, SHORT which, SHORT *w, SHORT *h)
{
    DPRINTF("_GTBUTTON_domain_cb: gtg=0x%08lx, which=%d\n", gtg, which);

    // FIXME: implement
    *w = 100;
    *h = 23;
}

void _GTBUTTON_CONSTRUCTOR (GTBUTTON_t *this, GTGADGET_t *parent,
                            char *txt, SHORT id,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTBUTTON_CONSTRUCTOR: this=0x%08lx, id=%d, txt=%s\n", this, id, txt ? txt : "NULL");
    _GTGADGET_CONSTRUCTOR (&this->gadget, parent, txt, id, user_data, flags, underscore);
    this->gadget.deploy_cb = _GTBUTTON_deploy_cb;
    this->gadget.domain_cb = _GTBUTTON_domain_cb;
    this->disabled         = FALSE;
}

BOOL _GTBUTTON_disabled_ (GTBUTTON_t *this)
{
    return this->disabled;
}
void _GTBUTTON_disabled (GTBUTTON_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        struct Window *win = _aqb_get_win(this->gadget.win_id);
        GT_SetGadgetAttrs (this->gadget.gad, win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
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

static void window_close_cb (short win_id, void *ud)
{
    DPRINTF ("GadToolsSupport: window_close_cb called on win #%d\n", win_id);

    ui_win_ext_t *ext = &g_win_ext[win_id-1];
    struct Window *win = _aqb_get_win(win_id-1);
    _gtgadgets_free (win, ext);
}

static void _gtgadgets_deploy (short win_id);

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

            case IDCMP_NEWSIZE:
            {
                SHORT win_id = _aqb_get_win_id (win);

                _gtgadgets_deploy (win_id);

                // FIXME handled = TRUE;
                break;
            }
        }
    }

    GT_PostFilterIMsg (imsg);

    return handled;
}

static void _gtgadgets_deploy (short win_id)
{
    DPRINTF ("_gadgets_deploy called, win_id=%d\n", win_id);
    ui_win_ext_t *ext = &g_win_ext[win_id-1];
    struct Window *win = _aqb_get_win(win_id-1);

    DPRINTF ("_gtgadgets_deploy: ext=0x%08lx, win=0x%08lx\n", ext, win);

    if (ext->deployed)
    {
		DPRINTF ("_gtgadgets_deploy: already deployed -> redeploy\n");
        _gtgadgets_free (win, ext);
        Move (_g_cur_rp, 0, 0);
        ClearScreen(_g_cur_rp);
        ext->deployed = FALSE;
    }

	if (!ext->close_cb_installed)
	{
		DPRINTF ("_gtgadgets_deploy: installing custom close callback for the current window\n");
		_window_add_close_cb (window_close_cb, NULL);
		ext->close_cb_installed = TRUE;
	}

    if (!ext->vinfo)
    {
        ext->vinfo = GetVisualInfo(_g_cur_win->WScreen, TAG_END);
        if (!ext->vinfo)
        {
            DPRINTF ("_gtgadgets_deploy: GetVisualInfo() failed.\n");
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
        DPRINTF ("_gtgadgets_deploy: CreateContext() failed.\n");
        ERROR(AE_GTG_CREATE);
        return;
    }

    GTGADGET_t *gtg = ext->root;
    if (gtg)
    {
        if (gtg->deploy_cb)
        {
            SHORT x, y, w, h;
            if (win->Flags & WFLG_GIMMEZEROZERO)
            {
                x=0;
                y=0;
                w=win->GZZWidth;
                h=win->GZZHeight;
            }
            else
            {
                x=win->BorderLeft;
                y=win->BorderTop;
                w=win->Width - win->BorderLeft - win->BorderRight;
                h=win->Height - win->BorderTop - win->BorderBottom;
            }
            DPRINTF ("_gtgadgets_deploy: ->deploy win=0x%08lx x=%d, y=%d, w=%d, h=%d\n", win, x, y, w, h);
            ext->gad = gtg->deploy_cb(gtg, ext->gad, ext->vinfo, &ext->ta, x, y, w, h);
        }
        else
        {
            ext->gad = NULL;
        }
    }

    if (!ext->msg_cb_installed)
    {
        DPRINTF ("GTGADGET_: installing custom msg callback for the current window\n");
        _window_add_msg_cb (window_msg_cb);
        ext->msg_cb_installed = TRUE;

        // take care of IDCMP flags
        ULONG widcmp = IDCMP_NEWSIZE;

        if ((win->IDCMPFlags & widcmp) != widcmp )
            ModifyIDCMP (win, win->IDCMPFlags | widcmp);

    }

    AddGList (_g_cur_win, ext->gadList, 1000, -1, NULL);
    GT_RefreshWindow (_g_cur_win, NULL);
    RefreshGadgets (ext->gadList, _g_cur_win, NULL);
    GT_BeginRefresh (_g_cur_win);
    GT_EndRefresh (_g_cur_win, TRUE);

    ext->deployed = TRUE;
}

void GTGADGETS_DEPLOY (void)
{
    DPRINTF ("GADGETS_DEPLOY called\n");

    _aqb_get_output (/*needGfx=*/TRUE);

    _gtgadgets_deploy (_g_cur_win_id);

}

#if 0
void GTGADGETS_FREE (void)
{
    DPRINTF ("GADGETS_FREE called\n");

    _aqb_get_output (/*needGfx=*/TRUE);

    ui_win_ext_t *ext = &g_win_ext[_g_cur_win_id-1];

    _gtgadgets_free (_g_cur_win, ext);
}

void GTG_DRAW_BEVEL_BOX (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, BOOL recessed )
{
    DPRINTF ("GTG_DRAW_BEVEL_BOX: %d/%d - %d/%d recessed=%d\n", x1, y1, x2, y2, recessed);

    _aqb_get_output (/*needGfx=*/TRUE);

    ui_win_ext_t *ext = &g_win_ext[_g_cur_win_id-1];
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

        ext->root               = NULL;
        ext->gad                = NULL;
        ext->gadList            = NULL;
        ext->vinfo              = NULL;
        ext->msg_cb_installed   = FALSE;
        ext->close_cb_installed = FALSE;
        ext->deployed           = FALSE;
    }

    ON_EXIT_CALL(_GadToolsSupport_shutdown);
}

