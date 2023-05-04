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

static struct Gadget *_gtcycle_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTCycle *gt = (CGTCycle *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (CYCLE_KIND, gad, &gtg->ng,
                             GT_Underscore   , gtg->underscore,
                             GA_Disabled     , gt->disabled,
                             GTCY_Labels     , (intptr_t) gt->labels,
                             GTCY_Active     , gt->active,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtcycle_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = CYCLEIDCMP;

    DPRINTF("_gtcycle_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _CGTCYCLE_CONSTRUCTOR (CGTCycle *this,
                           CONST_STRPTR label, CONST_STRPTR * labels,
                           BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                           void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTCycle_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _CGTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtcycle_deploy_cb;
    this->labels           = labels;
    this->disabled         = FALSE;
    this->active           = 0;
}

BOOL _CGTCYCLE_DISABLED_ (CGTCycle *this)
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
void _CGTCYCLE_DISABLED (CGTCycle *this, BOOL disabled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
}

CONST_STRPTR * _CGTCYCLE_LABELS_ (CGTCycle *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTCY_Labels, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return (CONST_STRPTR *)u;
    }
    return this->labels;
}
void _CGTCYCLE_LABELS (CGTCycle *this, CONST_STRPTR * labels)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTCY_Labels, (intptr_t) labels, TAG_DONE);
    }
    this->labels = labels;
}

USHORT _CGTCYCLE_ACTIVE_ (CGTCycle *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTCY_Active, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->active;
}
void _CGTCYCLE_ACTIVE (CGTCycle *this, USHORT active)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTCY_Active, active, TAG_DONE);
    }
    this->active = active;
}

static intptr_t _CGTCycle_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CGTCYCLE___init (CGTCycle *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTCycle_vtable;
}

