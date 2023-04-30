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

static struct Gadget *_gtnumber_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTNumber *gt = (CGTNumber *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (NUMBER_KIND, gad, &gtg->ng,
                             GT_Underscore   , gtg->underscore,
                             GTNM_Number     , gt->number,
                             GTNM_Border     , gt->border,
                             GTNM_FrontPen   , gt->frontPen,
                             GTNM_BackPen    , gt->backPen,
                             GTNM_Justification , gt->justification,
                             GTNM_Format     , (intptr_t) gt->format,
                             GTNM_MaxNumberLen , gt->maxNumberLen,
                             GTNM_Clipped    , gt->clipped,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtnumber_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = NUMBERIDCMP;

    DPRINTF("_gtnumber_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _CGTNUMBER_CONSTRUCTOR (CGTNumber *this,
                            CONST_STRPTR label, LONG number,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTNumber_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _CGTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtnumber_deploy_cb;
    this->number          = number;
    this->border          = TRUE;
    this->frontPen        = 1;
    this->backPen         = 0;
    this->justification   = GTJ_LEFT;
    this->format          = (CONST_STRPTR) "%ld";
    this->maxNumberLen    = 10;
    this->clipped         = TRUE;
}

LONG _CGTNUMBER_NUMBER_ (CGTNumber *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTNM_Number, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->number;
}
void _CGTNUMBER_NUMBER (CGTNumber *this, LONG number)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Number, number, TAG_DONE);
    }
    this->number = number;
}

BOOL _CGTNUMBER_BORDER_ (CGTNumber *this)
{
    return this->border;
}
void _CGTNUMBER_BORDER (CGTNumber *this, BOOL border)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Border, border, TAG_DONE);
    }
    this->border = border;
}

UBYTE _CGTNUMBER_FRONTPEN_ (CGTNumber *this)
{
    return this->frontPen;
}
void _CGTNUMBER_FRONTPEN (CGTNumber *this, UBYTE frontPen)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_FrontPen, frontPen, TAG_DONE);
    }
    this->frontPen = frontPen;
}

UBYTE _CGTNUMBER_BACKPEN_ (CGTNumber *this)
{
    return this->backPen;
}
void _CGTNUMBER_BACKPEN (CGTNumber *this, UBYTE backPen)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_BackPen, backPen, TAG_DONE);
    }
    this->backPen = backPen;
}

UBYTE _CGTNUMBER_JUSTIFICATION_ (CGTNumber *this)
{
    return this->justification;
}
void _CGTNUMBER_JUSTIFICATION (CGTNumber *this, UBYTE justification)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Justification, justification, TAG_DONE);
    }
    this->justification = justification;
}

CONST_STRPTR _CGTNUMBER_FORMAT_ (CGTNumber *this)
{
    return this->format;
}
void _CGTNUMBER_FORMAT (CGTNumber *this, CONST_STRPTR format)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Format, (intptr_t) format, TAG_DONE);
    }
    this->format = format;
}

ULONG _CGTNUMBER_MAXNUMBERLEN_ (CGTNumber *this)
{
    return this->maxNumberLen;
}
void _CGTNUMBER_MAXNUMBERLEN (CGTNumber *this, ULONG maxNumberLen)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_MaxNumberLen, maxNumberLen, TAG_DONE);
    }
    this->maxNumberLen = maxNumberLen;
}

BOOL _CGTNUMBER_CLIPPED_ (CGTNumber *this)
{
    return this->clipped;
}
void _CGTNUMBER_CLIPPED (CGTNumber *this, BOOL clipped)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Clipped, clipped, TAG_DONE);
    }
    this->clipped = clipped;
}

static intptr_t _CGTNumber_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

VOID _CGTNUMBER___init (CGTNumber *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTNumber_vtable;
}

