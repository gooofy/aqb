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


struct Gadget *_gtbutton_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTBUTTON_t *button = (GTBUTTON_t *)gtg;

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

void _GTBUTTON_CONSTRUCTOR (GTBUTTON_t *this, CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTBUTTON_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d, label=%s\n", this, x1, y1, x2, y2, label ? label : "NULL");
    _GTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtbutton_deploy_cb;
    this->disabled         = FALSE;
}

BOOL _GTBUTTON_disabled_ (GTBUTTON_t *this)
{
    return this->disabled;
}
void _GTBUTTON_disabled (GTBUTTON_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    this->disabled = disabled;
}

BOOL _GTBUTTON_immediate_ (GTBUTTON_t *this)
{
    return this->immediate;
}
void _GTBUTTON_immediate (GTBUTTON_t *this, BOOL immediate)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Immediate, immediate, TAG_DONE);
    }
    this->immediate = immediate;
}

