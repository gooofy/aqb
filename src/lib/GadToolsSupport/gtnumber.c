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

static struct Gadget *_gtnumber_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTNUMBER_t *gt = (GTNUMBER_t *)gtg;

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

void _GTNUMBER_CONSTRUCTOR (GTNUMBER_t *this,
                            CONST_STRPTR label, LONG number,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTNUMBER_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _GTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
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

LONG _GTNUMBER_number_ (GTNUMBER_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTNM_Number, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->number;
}
void _GTNUMBER_number (GTNUMBER_t *this, LONG number)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Number, number, TAG_DONE);
    }
    this->number = number;
}

BOOL _GTNUMBER_border_ (GTNUMBER_t *this)
{
    return this->border;
}
void _GTNUMBER_border (GTNUMBER_t *this, BOOL border)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Border, border, TAG_DONE);
    }
    this->border = border;
}

UBYTE _GTNUMBER_frontPen_ (GTNUMBER_t *this)
{
    return this->frontPen;
}
void _GTNUMBER_frontPen (GTNUMBER_t *this, UBYTE frontPen)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_FrontPen, frontPen, TAG_DONE);
    }
    this->frontPen = frontPen;
}

UBYTE _GTNUMBER_backPen_ (GTNUMBER_t *this)
{
    return this->backPen;
}
void _GTNUMBER_backPen (GTNUMBER_t *this, UBYTE backPen)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_BackPen, backPen, TAG_DONE);
    }
    this->backPen = backPen;
}

UBYTE _GTNUMBER_justification_ (GTNUMBER_t *this)
{
    return this->justification;
}
void _GTNUMBER_justification (GTNUMBER_t *this, UBYTE justification)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Justification, justification, TAG_DONE);
    }
    this->justification = justification;
}

CONST_STRPTR _GTNUMBER_format_ (GTNUMBER_t *this)
{
    return this->format;
}
void _GTNUMBER_format (GTNUMBER_t *this, CONST_STRPTR format)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Format, (intptr_t) format, TAG_DONE);
    }
    this->format = format;
}

ULONG _GTNUMBER_maxNumberLen_ (GTNUMBER_t *this)
{
    return this->maxNumberLen;
}
void _GTNUMBER_maxNumberLen (GTNUMBER_t *this, ULONG maxNumberLen)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_MaxNumberLen, maxNumberLen, TAG_DONE);
    }
    this->maxNumberLen = maxNumberLen;
}

BOOL _GTNUMBER_clipped_ (GTNUMBER_t *this)
{
    return this->clipped;
}
void _GTNUMBER_clipped (GTNUMBER_t *this, BOOL clipped)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTNM_Clipped, clipped, TAG_DONE);
    }
    this->clipped = clipped;
}
