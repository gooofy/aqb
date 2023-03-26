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

static struct Gadget *_gtcheckbox_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTCHECKBOX_t *cb = (GTCHECKBOX_t *)gtg;

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

void _GTCHECKBOX_CONSTRUCTOR (GTCHECKBOX_t *this, char *txt,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTCHECKBOX_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d, txt=%s\n", this, x1, y1, x2, y2, txt ? txt : "NULL");
    _GTGADGET_CONSTRUCTOR (&this->gadget, txt, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtcheckbox_deploy_cb;
    this->disabled         = FALSE;
    this->checked          = FALSE;
}

BOOL _GTCHECKBOX_disabled_ (GTCHECKBOX_t *this)
{
    return this->disabled;
}
void _GTCHECKBOX_disabled (GTCHECKBOX_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    this->disabled = disabled;
}

BOOL _GTCHECKBOX_checked_ (GTCHECKBOX_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget))
        return this->gadget.gad->Flags & GFLG_SELECTED;
    return this->checked;
}

void _GTCHECKBOX_checked (GTCHECKBOX_t *this, BOOL checked)
{
    DPRINTF ("_GTCHECKBOX_checked: this=0x%08x, checked=%d, size=%d\n", this, checked, sizeof (GTCHECKBOX_t));
    #if 1
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        DPRINTF ("_GTCHECKBOX_checked: is deployed, gad=0x%08x\n", this->gadget.gad);
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTCB_Checked, checked, TAG_DONE);
    }
    this->checked = checked;
    DPRINTF ("_GTCHECKBOX_checked: this=0x%08x, this->checked=%d\n", this, this->checked);
    #endif
}


