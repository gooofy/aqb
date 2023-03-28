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

static struct Gadget *_gtinteger_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTINTEGER_t *gt = (GTINTEGER_t *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (INTEGER_KIND, gad, &gtg->ng,
                             GT_Underscore   , gtg->underscore,
                             GA_Disabled     , gt->disabled,
                             GA_Immediate    , gt->immediate,
                             GA_TabCycle     , gt->tabCycle,
                             GTIN_Number     , gt->number,
                             GTIN_MaxChars   , gt->maxChars,
                             STRINGA_ExitHelp , gt->exitHelp,
                             STRINGA_Justification , (intptr_t) gt->justification,
                             STRINGA_ReplaceMode , gt->replaceMode,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtinteger_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = INTEGERIDCMP;

    DPRINTF("_gtinteger_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _GTINTEGER_CONSTRUCTOR (GTINTEGER_t *this,
                            CONST_STRPTR label, 
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTINTEGER_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _GTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtinteger_deploy_cb;
    this->disabled        = FALSE;
    this->immediate       = FALSE;
    this->tabCycle        = TRUE;
    this->number          = 0;
    this->maxChars        = 10;
    this->exitHelp        = FALSE;
    this->justification   = GACT_STRINGLEFT;
    this->replaceMode     = FALSE;
}

BOOL _GTINTEGER_disabled_ (GTINTEGER_t *this)
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
void _GTINTEGER_disabled (GTINTEGER_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
}

BOOL _GTINTEGER_immediate_ (GTINTEGER_t *this)
{
    return this->immediate;
}
void _GTINTEGER_immediate (GTINTEGER_t *this, BOOL immediate)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Immediate, immediate, TAG_DONE);
    }
    this->immediate = immediate;
}

BOOL _GTINTEGER_tabCycle_ (GTINTEGER_t *this)
{
    return this->tabCycle;
}
void _GTINTEGER_tabCycle (GTINTEGER_t *this, BOOL tabCycle)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_TabCycle, tabCycle, TAG_DONE);
    }
    this->tabCycle = tabCycle;
}

LONG _GTINTEGER_number_ (GTINTEGER_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTIN_Number, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->number;
}
void _GTINTEGER_number (GTINTEGER_t *this, LONG number)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTIN_Number, number, TAG_DONE);
    }
    this->number = number;
}

USHORT _GTINTEGER_maxChars_ (GTINTEGER_t *this)
{
    return this->maxChars;
}
void _GTINTEGER_maxChars (GTINTEGER_t *this, USHORT maxChars)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTIN_MaxChars, maxChars, TAG_DONE);
    }
    this->maxChars = maxChars;
}

BOOL _GTINTEGER_exitHelp_ (GTINTEGER_t *this)
{
    return this->exitHelp;
}
void _GTINTEGER_exitHelp (GTINTEGER_t *this, BOOL exitHelp)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, STRINGA_ExitHelp, exitHelp, TAG_DONE);
    }
    this->exitHelp = exitHelp;
}

CONST_STRPTR _GTINTEGER_justification_ (GTINTEGER_t *this)
{
    return this->justification;
}
void _GTINTEGER_justification (GTINTEGER_t *this, CONST_STRPTR justification)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, STRINGA_Justification, (intptr_t) justification, TAG_DONE);
    }
    this->justification = justification;
}

BOOL _GTINTEGER_replaceMode_ (GTINTEGER_t *this)
{
    return this->replaceMode;
}
void _GTINTEGER_replaceMode (GTINTEGER_t *this, BOOL replaceMode)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, STRINGA_ReplaceMode, replaceMode, TAG_DONE);
    }
    this->replaceMode = replaceMode;
}
