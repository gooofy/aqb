#define ENABLE_DPRINTF

#include "../_aqb/_aqb.h"
#include "../_brt/_brt.h"

#include "IntuiSupport.h"

#include <exec/types.h>
//#include <exec/memory.h>
//#include <clib/exec_protos.h>
//#include <inline/exec.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <clib/intuition_protos.h>
#include <inline/intuition.h>

#include <clib/graphics_protos.h>
#include <inline/graphics.h>


VOID _CMENUITEMSEPARATOR_CONSTRUCTOR (CMenuItemSeparator *THIS, CMenu *parent, intptr_t userData)
{
    _CMENUITEM_CONSTRUCTOR ((CMenuItem *) THIS, parent, userData);

    //THIS->_item._item.Flags &= ~ITEMENABLED;
    //THIS->_item._item.Flags &= ~HIGHCOMP;
    //THIS->_item._item.Flags |= HIGHNONE;
    THIS->_item._item.Flags = HIGHNONE;

    struct Image *img = (struct Image*) ALLOCATE_ (sizeof(*img), 0);
    if (!img)
        ERROR (ERR_OUT_OF_MEMORY);
    intuis_win_ext_t *ext = _IntuiSupport_get_ext(_g_cur_win_id);

    img->LeftEdge   = 2;
    img->TopEdge    = 2;
    img->Width      = 0;
    img->Height     = 2;
    img->Depth      = 0;
    img->ImageData  = NULL;
    img->PlanePick  = 0;
    img->PlaneOnOff = ext->draw_info->dri_Pens[BARDETAILPEN];
    img->NextImage  = NULL;

    THIS->_item._item.ItemFill   = img;
    THIS->_item._item.SelectFill = NULL;
}


static intptr_t _CMenuItemSeparator_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CMENUITEMSEPARATOR___init (CMenuItemSeparator *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CMenuItemSeparator_vtable;
}

