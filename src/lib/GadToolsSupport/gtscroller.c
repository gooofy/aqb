//#define ENABLE_DPRINTF

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

extern struct Library    *GadToolsBase ;

static struct Gadget *_gtscroller_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTSCROLLER_t *gt = (GTSCROLLER_t *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (SCROLLER_KIND, gad, &gtg->ng,
                             GT_Underscore   , gtg->underscore,
                             GA_Disabled     , gt->disabled,
                             GA_RelVerify    , gt->relVerify,
                             GA_Immediate    , gt->immediate,
                             GTSC_Top        , gt->top,
                             GTSC_Total      , gt->total,
                             GTSC_Visible    , gt->visible,
                             GTSC_Arrows     , gt->arrows,
                             PGA_Freedom     , gt->freedom,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtscroller_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = SCROLLERIDCMP;

    DPRINTF("_gtscroller_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _GTSCROLLER_CONSTRUCTOR (GTSCROLLER_t *this, CONST_STRPTR label,
                              SHORT top, SHORT total, SHORT visible, ULONG freedom,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTSCROLLER_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _GTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtscroller_deploy_cb;
    this->top             = top;
    this->total           = total;
    this->visible         = visible;
    this->disabled        = FALSE;
    this->relVerify       = FALSE;
    this->immediate       = FALSE;
    this->arrows          = 18;
    DPRINTF("__GTSCROLLER_CONSTRUCTOR: top=%d, total=%d, visible=%d\n", top, total, visible);
}

BOOL _GTSCROLLER_disabled_ (GTSCROLLER_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GA_Disabled, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->disabled;
}
void _GTSCROLLER_disabled (GTSCROLLER_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
}

BOOL _GTSCROLLER_relVerify_ (GTSCROLLER_t *this)
{
    return this->relVerify;
}
void _GTSCROLLER_relVerify (GTSCROLLER_t *this, BOOL relVerify)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_RelVerify, relVerify, TAG_DONE);
    }
    this->relVerify = relVerify;
}

BOOL _GTSCROLLER_immediate_ (GTSCROLLER_t *this)
{
    return this->immediate;
}
void _GTSCROLLER_immediate (GTSCROLLER_t *this, BOOL immediate)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Immediate, immediate, TAG_DONE);
    }
    this->immediate = immediate;
}

SHORT _GTSCROLLER_top_ (GTSCROLLER_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSC_Top, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->top;
}
void _GTSCROLLER_top (GTSCROLLER_t *this, SHORT top)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSC_Top, top, TAG_DONE);
    }
    this->top = top;
}

SHORT _GTSCROLLER_total_ (GTSCROLLER_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSC_Total, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->total;
}
void _GTSCROLLER_total (GTSCROLLER_t *this, SHORT total)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSC_Total, total, TAG_DONE);
    }
    this->total = total;
}

BOOL _GTSCROLLER_visible_ (GTSCROLLER_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSC_Visible, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->visible;
}
void _GTSCROLLER_visible (GTSCROLLER_t *this, BOOL visible)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSC_Visible, visible, TAG_DONE);
    }
    this->visible = visible;
}

USHORT _GTSCROLLER_arrows_ (GTSCROLLER_t *this)
{
    return this->arrows;
}
void _GTSCROLLER_arrows (GTSCROLLER_t *this, USHORT arrows)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSC_Arrows, arrows, TAG_DONE);
    }
    this->arrows = arrows;
}

ULONG _GTSCROLLER_freedom_ (GTSCROLLER_t *this)
{
    return this->freedom;
}
void _GTSCROLLER_freedom (GTSCROLLER_t *this, ULONG freedom)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, PGA_Freedom, freedom, TAG_DONE);
    }
    this->freedom = freedom;
}
