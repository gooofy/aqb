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


struct Gadget *_gtbutton_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTButton *button = (CGTButton *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (BUTTON_KIND, gad, &gtg->ng,
                             GA_Disabled     , button->disabled,
                             GA_Immediate    , button->immediate,
                             GT_Underscore   , gtg->underscore,
                             TAG_DONE);

	if (!gtg->gad)
	{
		DPRINTF ("_gtbutton_deploy_cb: CreateGadget() failed.\n");
		ERROR(AE_GTG_CREATE);
		return gad;
	}

    // take care of IDCMP flags
    ULONG gidcmp = BUTTONIDCMP;

	DPRINTF("_gtbutton_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

	if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
		ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _CGTBUTTON_CONSTRUCTOR (CGTButton *this, CONST_STRPTR label,
                             BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                             void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTButton_CONSTRUCTOR: this=0x%08lx (size=%d), x1=%d, y1=%d, x2=%d, y2=%d, label=%s\n",
            this, sizeof(*this), x1, y1, x2, y2, label ? label : (CONST_STRPTR)"NULL");
    DPRINTF("_CGTButton_CONSTRUCTOR: sizeof(struct NewGadget)=%d\n", sizeof(struct NewGadget));
    DPRINTF("_CGTButton_CONSTRUCTOR: disabled offset=%d, immediate offset=%d\n",
            ((intptr_t)&this->disabled-(intptr_t)this),
            ((intptr_t)&this->immediate-(intptr_t)this));
    _CGTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtbutton_deploy_cb;
    this->disabled         = FALSE;
}

BOOL _CGTBUTTON_DISABLED_ (CGTButton *this)
{
    return this->disabled;
}
void _CGTBUTTON_DISABLED (CGTButton *this, BOOL disabled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    this->disabled = disabled;
}

BOOL _CGTBUTTON_IMMEDIATE_ (CGTButton *this)
{
    return this->immediate;
}
void _CGTBUTTON_IMMEDIATE (CGTButton *this, BOOL immediate)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Immediate, immediate, TAG_DONE);
    }
    this->immediate = immediate;
}

static intptr_t _CGTButton_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CGTBUTTON___init (CGTButton *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTButton_vtable;
}

