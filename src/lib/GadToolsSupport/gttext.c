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

static struct Gadget *_gttext_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTTEXT_t *gt = (GTTEXT_t *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (TEXT_KIND, gad, &gtg->ng,
                             GT_Underscore   , gtg->underscore,
                             GTTX_Text       , (intptr_t) gt->text,
                             GTTX_CopyText   , gt->copyText,
                             GTTX_Border     , gt->border,
                             GTTX_FrontPen   , gt->frontPen,
                             GTTX_BackPen    , gt->backPen,
                             GTTX_Justification , gt->justification,
                             GTTX_Clipped    , gt->clipped,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gttext_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = TEXTIDCMP;

    DPRINTF("_gttext_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _GTTEXT_CONSTRUCTOR (GTTEXT_t *this, CONST_STRPTR label,
                          CONST_STRPTR text,
                          BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                          void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTTEXT_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _GTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gttext_deploy_cb;
    this->text            = text;
    this->copyText        = FALSE;
    this->border          = TRUE;
    this->frontPen        = 1;
    this->backPen         = 0;
    this->justification   = GTJ_LEFT;
    this->clipped         = TRUE;
}

CONST_STRPTR _GTTEXT_text_ (GTTEXT_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTTX_Text, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return (CONST_STRPTR) (intptr_t) u;
    }
    return this->text;
}
void _GTTEXT_text (GTTEXT_t *this, CONST_STRPTR text)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTTX_Text, (intptr_t) text, TAG_DONE);
    }
    this->text = text;
}

BOOL _GTTEXT_copyText_ (GTTEXT_t *this)
{
    return this->copyText;
}
void _GTTEXT_copyText (GTTEXT_t *this, BOOL copyText)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTTX_CopyText, copyText, TAG_DONE);
    }
    this->copyText = copyText;
}

BOOL _GTTEXT_border_ (GTTEXT_t *this)
{
    return this->border;
}
void _GTTEXT_border (GTTEXT_t *this, BOOL border)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTTX_Border, border, TAG_DONE);
    }
    this->border = border;
}

UBYTE _GTTEXT_frontPen_ (GTTEXT_t *this)
{
    return this->frontPen;
}
void _GTTEXT_frontPen (GTTEXT_t *this, UBYTE frontPen)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTTX_FrontPen, frontPen, TAG_DONE);
    }
    this->frontPen = frontPen;
}

UBYTE _GTTEXT_backPen_ (GTTEXT_t *this)
{
    return this->backPen;
}
void _GTTEXT_backPen (GTTEXT_t *this, UBYTE backPen)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTTX_BackPen, backPen, TAG_DONE);
    }
    this->backPen = backPen;
}

UBYTE _GTTEXT_justification_ (GTTEXT_t *this)
{
    return this->justification;
}
void _GTTEXT_justification (GTTEXT_t *this, UBYTE justification)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTTX_Justification, justification, TAG_DONE);
    }
    this->justification = justification;
}

BOOL _GTTEXT_clipped_ (GTTEXT_t *this)
{
    return this->clipped;
}
void _GTTEXT_clipped (GTTEXT_t *this, BOOL clipped)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTTX_Clipped, clipped, TAG_DONE);
    }
    this->clipped = clipped;
}
