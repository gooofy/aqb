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

static struct Gadget *_gtscroller_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTScroller *gt = (CGTScroller *)gtg;

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

void _CGTSCROLLER_CONSTRUCTOR (CGTScroller *this, CONST_STRPTR label,
                              SHORT top, SHORT total, SHORT visible, ULONG freedom,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTScroller_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _CGTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtscroller_deploy_cb;
    this->top             = top;
    this->total           = total;
    this->visible         = visible;
    this->disabled        = FALSE;
    this->relVerify       = FALSE;
    this->immediate       = FALSE;
    this->arrows          = 18;
    DPRINTF("__CGTScroller_CONSTRUCTOR: top=%d, total=%d, visible=%d\n", top, total, visible);
}

BOOL _CGTSCROLLER_DISABLED_ (CGTScroller *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GA_Disabled, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->disabled;
}
void _CGTSCROLLER_DISABLED (CGTScroller *this, BOOL disabled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
}

BOOL _CGTSCROLLER_RELVERIFY_ (CGTScroller *this)
{
    return this->relVerify;
}
void _CGTSCROLLER_RELVERIFY (CGTScroller *this, BOOL relVerify)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_RelVerify, relVerify, TAG_DONE);
    }
    this->relVerify = relVerify;
}

BOOL _CGTSCROLLER_IMMEDIATE_ (CGTScroller *this)
{
    return this->immediate;
}
void _CGTSCROLLER_IMMEDIATE (CGTScroller *this, BOOL immediate)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Immediate, immediate, TAG_DONE);
    }
    this->immediate = immediate;
}

SHORT _CGTSCROLLER_TOP_ (CGTScroller *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSC_Top, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->top;
}
void _CGTSCROLLER_TOP (CGTScroller *this, SHORT top)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSC_Top, top, TAG_DONE);
    }
    this->top = top;
}

SHORT _CGTSCROLLER_TOTAL_ (CGTScroller *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSC_Total, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->total;
}
void _CGTSCROLLER_TOTAL (CGTScroller *this, SHORT total)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSC_Total, total, TAG_DONE);
    }
    this->total = total;
}

BOOL _CGTSCROLLER_VISIBLE_ (CGTScroller *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSC_Visible, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->visible;
}
void _CGTSCROLLER_VISIBLE (CGTScroller *this, BOOL visible)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSC_Visible, visible, TAG_DONE);
    }
    this->visible = visible;
}

USHORT _CGTSCROLLER_ARROWS_ (CGTScroller *this)
{
    return this->arrows;
}
void _CGTSCROLLER_ARROWS (CGTScroller *this, USHORT arrows)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSC_Arrows, arrows, TAG_DONE);
    }
    this->arrows = arrows;
}

ULONG _CGTSCROLLER_FREEDOM_ (CGTScroller *this)
{
    return this->freedom;
}
void _CGTSCROLLER_FREEDOM (CGTScroller *this, ULONG freedom)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, PGA_Freedom, freedom, TAG_DONE);
    }
    this->freedom = freedom;
}

static intptr_t _CGTScroller_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CGTSCROLLER___init (CGTScroller *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTScroller_vtable;
}

