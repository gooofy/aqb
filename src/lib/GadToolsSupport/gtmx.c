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

static struct Gadget *_gtmx_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTMX *gt = (CGTMX *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (MX_KIND, gad, &gtg->ng,
                             GT_Underscore   , gtg->underscore,
                             GA_Disabled     , gt->disabled,
                             GTMX_Labels     , (intptr_t) gt->labels,
                             GTMX_Active     , gt->active,
                             GTMX_Spacing    , gt->spacing,
                             GTMX_Scaled     , gt->scaled,
                             GTMX_TitlePlace , gt->titlePlace,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtmx_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = MXIDCMP;

    DPRINTF("_gtmx_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _CGTMX_CONSTRUCTOR (CGTMX *this,
                            CONST_STRPTR label, CONST_STRPTR * labels, 
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTMX_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _CGTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtmx_deploy_cb;
    this->labels          = labels;
    this->disabled        = FALSE;
    this->active          = 0;
    this->spacing         = 1;
    this->scaled          = FALSE;
    this->titlePlace      = PLACETEXT_LEFT;
}

BOOL _CGTMX_DISABLED_ (CGTMX *this)
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
void _CGTMX_DISABLED (CGTMX *this, BOOL disabled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
}

CONST_STRPTR * _CGTMX_LABELS_ (CGTMX *this)
{
    return this->labels;
}
void _CGTMX_LABELS (CGTMX *this, CONST_STRPTR * labels)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTMX_Labels, (intptr_t) labels, TAG_DONE);
    }
    this->labels = labels;
}

USHORT _CGTMX_ACTIVE_ (CGTMX *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTMX_Active, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->active;
}
void _CGTMX_ACTIVE (CGTMX *this, USHORT active)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTMX_Active, active, TAG_DONE);
    }
    this->active = active;
}

USHORT _CGTMX_SPACING_ (CGTMX *this)
{
    return this->spacing;
}
void _CGTMX_SPACING (CGTMX *this, USHORT spacing)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTMX_Spacing, spacing, TAG_DONE);
    }
    this->spacing = spacing;
}

BOOL _CGTMX_SCALED_ (CGTMX *this)
{
    return this->scaled;
}
void _CGTMX_SCALED (CGTMX *this, BOOL scaled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTMX_Scaled, scaled, TAG_DONE);
    }
    this->scaled = scaled;
}

ULONG _CGTMXITLEPLACE_ (CGTMX *this)
{
    return this->titlePlace;
}
void _CGTMXITLEPLACE (CGTMX *this, ULONG titlePlace)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTMX_TitlePlace, titlePlace, TAG_DONE);
    }
    this->titlePlace = titlePlace;
}

static intptr_t _CGTMX_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CGTMX___init (CGTMX *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTMX_vtable;
}

