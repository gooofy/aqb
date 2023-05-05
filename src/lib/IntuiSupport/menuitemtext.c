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


VOID _CMENUITEMTEXT_CONSTRUCTOR (CMenuItemText *THIS, STRPTR text, CMenu *parent, intptr_t userData)
{
    _CMENUITEM_CONSTRUCTOR ((CMenuItem *) THIS, parent, userData);

    THIS->_item.Flags |= ITEMTEXT;

    if (!text)
        text = NULL;

    _CMENUITEMTEXT_TEXT (THIS, text);
}

VOID _CMENUITEMTEXT_TEXT (CMenuItemText *THIS, STRPTR   s)
{
    if (!(THIS->_item.Flags & ITEMTEXT))
    {
        THIS->_item.ItemFill   = NULL;
        THIS->_item.SelectFill = NULL;
    }

    struct IntuiText *it = THIS->_item.ItemFill;

    if (!it)
    {
        it = (struct IntuiText*) ALLOCATE_ (sizeof(*it), 0);
        intuis_win_ext_t *ext = _IntuiSupport_get_ext(_g_cur_win_id);
        it->FrontPen = ext->draw_info->dri_Pens[BARDETAILPEN];
        it->BackPen  = ext->draw_info->dri_Pens[BARBLOCKPEN];
        it->DrawMode = JAM1;
        it->LeftEdge = 1;
        it->TopEdge  = 0;
        it->IText    = NULL;
        it->NextText = NULL;

        THIS->_item.ItemFill = it;
        THIS->_item.Flags |= ITEMTEXT;
    }

    it->IText = s;

    DPRINTF ("_CMENUITEMTEXT_TEXT: THIS=0x%08lx, it=0x%08lx -> %s, NextText=0x%08lx, SelectFill=0x%08lx\n",
             THIS, it, s, it->NextText, THIS->_item.SelectFill);
}

STRPTR   _CMENUITEMTEXT_TEXT_ (CMenuItemText *THIS)
{
    struct IntuiText *it = THIS->_item.ItemFill;
    if (!it)
        return (STRPTR) "";
    return it->IText;
}

VOID _CMENUITEMTEXT_TEXTSELECTED (CMenuItemText *THIS, STRPTR   s)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItemText.textSelected");
}

STRPTR   _CMENUITEMTEXT_TEXTSELECTED_ (CMenuItemText *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItemText.textSelected");
    return NULL;
}

VOID _CMENUITEMTEXT_ITEXT (CMenuItemText *THIS, struct IntuiText *s)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItemText.IText");
}

struct IntuiText *_CMENUITEMTEXT_ITEXT_ (CMenuItemText *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItemText.IText");
    return NULL;
}

VOID _CMENUITEMTEXT_ITEXTSELECTED (CMenuItemText *THIS, struct IntuiText *s)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItemText.iTextSelected");
}

struct IntuiText *_CMENUITEMTEXT_ITEXTSELECTED_ (CMenuItemText *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItemText.iTextSelected");
    return NULL;
}

static intptr_t _CMenuItemText_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CMENUITEMTEXT___init (CMenuItemText *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CMenuItemText_vtable;
}

