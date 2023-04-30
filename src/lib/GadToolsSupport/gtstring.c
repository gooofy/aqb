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

static struct Gadget *_gtstring_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTString *gt = (CGTString *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (STRING_KIND, gad, &gtg->ng,
                             GT_Underscore   , gtg->underscore,
                             GA_Disabled     , gt->disabled,
                             GA_Immediate    , gt->immediate,
                             GA_TabCycle     , gt->tabCycle,
                             GTST_String     , (intptr_t) gt->str,
                             GTST_MaxChars   , gt->maxChars,
                             STRINGA_ExitHelp , gt->exitHelp,
                             STRINGA_Justification , (intptr_t) gt->justification,
                             STRINGA_ReplaceMode , gt->replaceMode,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtstring_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = STRINGIDCMP;

    DPRINTF("_gtstring_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _CGTSTRING_CONSTRUCTOR (CGTString *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTString_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _CGTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtstring_deploy_cb;
    this->disabled        = FALSE;
    this->immediate       = FALSE;
    this->tabCycle        = TRUE;
    this->str             = NULL;
    this->maxChars        = 256;
    this->exitHelp        = FALSE;
    this->justification   = GACT_STRINGLEFT;
    this->replaceMode     = FALSE;
}

BOOL _CGTSTRING_DISABLED_ (CGTString *this)
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
void _CGTSTRING_DISABLED (CGTString *this, BOOL disabled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
}

BOOL _CGTSTRING_IMMEDIATE_ (CGTString *this)
{
    return this->immediate;
}
void _CGTSTRING_IMMEDIATE (CGTString *this, BOOL immediate)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Immediate, immediate, TAG_DONE);
    }
    this->immediate = immediate;
}

BOOL _CGTSTRINGABCYCLE_ (CGTString *this)
{
    return this->tabCycle;
}
void _CGTSTRINGABCYCLE (CGTString *this, BOOL tabCycle)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_TabCycle, tabCycle, TAG_DONE);
    }
    this->tabCycle = tabCycle;
}

CONST_STRPTR _CGTSTRING_STR_ (CGTString *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        struct StringInfo * si = (struct StringInfo *)this->gadget.gad->SpecialInfo;
        return si->Buffer;
    }
    return this->str;
}
void _CGTSTRING_STR (CGTString *this, CONST_STRPTR str)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTST_String, (intptr_t) str, TAG_DONE);
    }
    this->str = str;
}

USHORT _CGTSTRING_MAXCHARS_ (CGTString *this)
{
    return this->maxChars;
}
void _CGTSTRING_MAXCHARS (CGTString *this, USHORT maxChars)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTST_MaxChars, maxChars, TAG_DONE);
    }
    this->maxChars = maxChars;
}

BOOL _CGTSTRING_EXITHELP_ (CGTString *this)
{
    return this->exitHelp;
}
void _CGTSTRING_EXITHELP (CGTString *this, BOOL exitHelp)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, STRINGA_ExitHelp, exitHelp, TAG_DONE);
    }
    this->exitHelp = exitHelp;
}

CONST_STRPTR _CGTSTRING_JUSTIFICATION_ (CGTString *this)
{
    return this->justification;
}
void _CGTSTRING_JUSTIFICATION (CGTString *this, CONST_STRPTR justification)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, STRINGA_Justification, (intptr_t) justification, TAG_DONE);
    }
    this->justification = justification;
}

BOOL _CGTSTRING_REPLACEMODE_ (CGTString *this)
{
    return this->replaceMode;
}
void _CGTSTRING_REPLACEMODE (CGTString *this, BOOL replaceMode)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, STRINGA_ReplaceMode, replaceMode, TAG_DONE);
    }
    this->replaceMode = replaceMode;
}

static intptr_t _CGTString_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CGTSTRING___init (CGTString *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTString_vtable;
}

