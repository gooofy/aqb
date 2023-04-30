// #define ENABLE_DPRINTF

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

void _CGTGADGET_CONSTRUCTOR (CGTGadget *this, CONST_STRPTR txt,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{

    DPRINTF("_CGTGadget_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, txt=%s\n", this, x1, y1, txt ? txt : (CONST_STRPTR)"NULL");

    gt_win_ext_t *ext = &_g_gt_win_ext[_g_cur_win_id];

    if (ext->deployed)
    {
		DPRINTF ("_CGTGadget_CONSTRUCTOR: already deployed\n");
		ERROR(AE_GTG_CREATE);
		return;
    }

    DPRINTF("_CGTGadget_CONSTRUCTOR: 0 ext=0x%08lx, ext->last=0x%08lx, ext->first=0x%08lx\n", ext, ext->last, ext->first);
    this->next = NULL;
    this->prev = ext->last;
    if (ext->last)
        ext->last = ext->last->next = this;
    else
        ext->first = ext->last = this;
    DPRINTF("_CGTGadget_CONSTRUCTOR: 1 ext=0x%08lx, ext->last=0x%08lx, ext->first=0x%08lx\n", ext, ext->last, ext->first);

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

SHORT _CGTGADGET_X1_ (CGTGadget *this)
{
    return this->ng.ng_LeftEdge;
}
void _CGTGADGET_X1 (CGTGadget *this, SHORT x1)
{
    this->ng.ng_LeftEdge = x1;
}

SHORT _CGTGADGET_Y1_ (CGTGadget *this)
{
    return this->ng.ng_TopEdge;
}
void _CGTGADGET_Y1 (CGTGadget *this, SHORT y1)
{
    this->ng.ng_TopEdge = y1;
}

SHORT _CGTGADGET_X2_ (CGTGadget *this)
{
    return this->ng.ng_LeftEdge + this->ng.ng_Width - 1;
}
void _CGTGADGET_X2 (CGTGadget *this, SHORT x2)
{
    this->ng.ng_Width = x2-this->ng.ng_LeftEdge+1;
}

SHORT _CGTGADGET_Y2_ (CGTGadget *this)
{
    return this->ng.ng_TopEdge + this->ng.ng_Height - 1;
}
void _CGTGADGET_Y2 (CGTGadget *this, SHORT y2)
{
    this->ng.ng_Height = y2-this->ng.ng_TopEdge+1;
}

CONST_STRPTR _CGTGADGET_TEXT_ (CGTGadget *this)
{
    return this->ng.ng_GadgetText;
}
void _CGTGADGET_TEXT (CGTGadget *this, STRPTR text)
{
    DPRINTF ("_CGTGadget_text: text=%s\n", text ? (char*)text : "NULL");
    this->ng.ng_GadgetText = text;
}

SHORT  _CGTGADGET_ID_ (CGTGadget *this)
{
    return this->ng.ng_GadgetID;
}
void _CGTGADGET_ID (CGTGadget *this, SHORT id)
{
    this->ng.ng_GadgetID = id;
}

ULONG _CGTGADGET_FLAGS_ (CGTGadget *this)
{
    return this->ng.ng_Flags;
}
void _CGTGADGET_FLAGS (CGTGadget *this, ULONG flags)
{
    this->ng.ng_Flags = flags;
}

BOOL _CGTGADGET_DEPLOYED_ (CGTGadget *this)
{
    return this->gad != NULL;
}

static void _gtgadgets_free (struct Window *win, gt_win_ext_t *ext)
{
    if (ext->deployed)
    {
		DPRINTF ("_gtgadgets_free: was deployed\n");
        RemoveGList (_g_cur_win, ext->gadList, -1);
        ext->deployed = FALSE;
    }

    if (ext->gadList)
    {
		DPRINTF ("_gtgadgets_free: g_gadList not null\n");
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

                CGTGadget *gtg = (CGTGadget *) g->UserData;
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

                CGTGadget *gtg = (CGTGadget *) g->UserData;
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

                CGTGadget *gtg = (CGTGadget *) g->UserData;
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
		DPRINTF ("GTGADGETS_DEPLOY: GetVisualInfo() for the current window\n");
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

    struct Gadget **pGadList = &ext->gadList;
    //DPRINTF("_GTGADGET_DEPLOY: CreateContext(), ext->gadList=0x%08lx, pGadList=0x%08lx\n", ext->gadList, pGadList);
    ext->gad = CreateContext (pGadList);
    if (!ext->gad)
    {
        DPRINTF ("GTGADGETS_DEPLOY: CreateContext() failed.\n");
        ERROR(AE_GTG_CREATE);
        return;
    }
    CGTGadget *gtg = ext->first;
    while (gtg)
    {
        //DPRINTF ("GTGADGETS_DEPLOY: gtg=0x%08lx, gtg->deploy_cb=0x%08lx\n", gtg, gtg->deploy_cb);
        if (gtg->deploy_cb)
        {
            //DPRINTF ("GTGADGETS_DEPLOY: gtg->deploy_cb()\n");
            ext->gad = gtg->deploy_cb(gtg, ext->gad, ext->vinfo, &ext->ta);
        }
        gtg = gtg->next;
    }

    if (!ext->msg_cb_installed)
    {
        DPRINTF ("GTGADGETS_DEPLOY: installing custom msg callback for the current window\n");
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

static intptr_t _CGTGadget_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CGTGADGET___init (CGTGadget *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CGTGadget_vtable;
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

    //DPRINTF("gadtools version=%d, revision=%d, id=%s\n",
    //        (int)GadToolsBase->lib_Version,
    //        (int)GadToolsBase->lib_Revision,
    //        GadToolsBase->lib_IdString);
}

