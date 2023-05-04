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

static struct Gadget *_gtcheckbox_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTCheckBox *cb = (CGTCheckBox *)gtg;

	DPRINTF("_gtcheckbox_deploy_cb: cb=0x%08lx, checked=%d\n", cb, cb->checked);

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (CHECKBOX_KIND, gad, &gtg->ng, GA_Disabled, cb->disabled, GT_Underscore, gtg->underscore, GTCB_Checked, cb->checked, TAG_DONE);

	if (!gtg->gad)
	{
		DPRINTF ("_gtcheckbox_deploy_cb: CreateGadget() failed.\n");
		ERROR(AE_GTG_CREATE);
		return gad;
	}

    // take care of IDCMP flags
    ULONG gidcmp = CHECKBOXIDCMP;

	DPRINTF("_gtcheckbox_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

	if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
		ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _CGTCHECKBOX_CONSTRUCTOR (CGTCheckBox *this, CONST_STRPTR label,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTCheckBox_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d, label=%s\n", this, x1, y1, x2, y2, label ? label : "NULL");
    _CGTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtcheckbox_deploy_cb;
    this->disabled         = FALSE;
    this->checked          = FALSE;
    this->scaled           = FALSE;
}

BOOL _CGTCHECKBOX_DISABLED_ (CGTCheckBox *this)
{
    return this->disabled;
}
void _CGTCHECKBOX_DISABLED (CGTCheckBox *this, BOOL disabled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    this->disabled = disabled;
}

BOOL _CGTCHECKBOX_CHECKED_ (CGTCheckBox *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
        return this->gadget.gad->Flags & GFLG_SELECTED;
    return this->checked;
}

void _CGTCHECKBOX_CHECKED (CGTCheckBox *this, BOOL checked)
{
    DPRINTF ("_CGTCheckBox_checked: this=0x%08x, checked=%d, size=%d\n", this, checked, sizeof (CGTCheckBox));
    #if 1
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        DPRINTF ("_CGTCheckBox_checked: is deployed, gad=0x%08x\n", this->gadget.gad);
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTCB_Checked, checked, TAG_DONE);
    }
    this->checked = checked;
    DPRINTF ("_CGTCheckBox_checked: this=0x%08x, this->checked=%d\n", this, this->checked);
    #endif
}

BOOL _CGTCHECKBOX_SCALED_ (CGTCheckBox *this)
{
    return this->scaled;
}

void _CGTCHECKBOX_SCALED (CGTCheckBox *this, BOOL scaled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTCB_Scaled, scaled, TAG_DONE);
    this->scaled = scaled;
}

static intptr_t _CGTCheckBox_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CGTCHECKBOX___init (CGTCheckBox *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTCheckBox_vtable;
}


