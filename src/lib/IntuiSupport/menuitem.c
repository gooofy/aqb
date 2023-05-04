// #define ENABLE_DPRINTF

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

VOID _CMENUITEM_CONSTRUCTOR (CMenuItem *THIS, STRPTR text, CMenu *parent)
{
    THIS->_cb                 = NULL;
    THIS->_item.NextItem      = NULL;
    THIS->_item.Flags         = ITEMENABLED | HIGHCOMP ;
    THIS->_item.MutualExclude = 0;
    THIS->_item.ItemFill      = NULL;
    THIS->_item.SelectFill    = NULL;
    THIS->_item.Command       = 0;
    THIS->_item.SubItem       = NULL;

    if (text)
        _CMENUITEM_TEXT (THIS, text);

    if (parent)
        _CMENU_ADDITEM (parent, THIS);
}

VOID _CMENUITEM_TEXT (CMenuItem *THIS, STRPTR s)
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
}

STRPTR   _CMENUITEM_TEXT_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.text");
    return NULL;
}

VOID _CMENUITEM_TEXTSELECTED (CMenuItem *THIS, STRPTR   s)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.textSelected");
}

STRPTR   _CMENUITEM_TEXTSELECTED_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.textSelected");
    return NULL;
}

VOID _CMENUITEM_ITEXT (CMenuItem *THIS, struct IntuiText *s)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.IText");
}

struct IntuiText *_CMENUITEM_ITEXT_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.IText");
    return NULL;
}

VOID _CMENUITEM_ITEXTSELECTED (CMenuItem *THIS, struct IntuiText *s)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.iTextSelected");
}

struct IntuiText *_CMENUITEM_ITEXTSELECTED_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.iTextSelected");
    return NULL;
}

VOID _CMENUITEM_IMAGE (CMenuItem *THIS, struct Image *s)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.Image");
}

struct Image *_CMENUITEM_IMAGE_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.Image");
    return NULL;
}

VOID _CMENUITEM_IMAGESELECTED (CMenuItem *THIS, struct Image *s)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.imageSelected");
}

struct Image *_CMENUITEM_IMAGESELECTED_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.imageSelected");
    return NULL;
}

VOID _CMENUITEM_CHECKIT (CMenuItem *THIS, BOOL     B)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.CHECKIT");
}

BOOL     _CMENUITEM_CHECKIT_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.CHECKIT");
    return FALSE;
}

VOID _CMENUITEM_CHECKED (CMenuItem *THIS, BOOL     B)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.CHECKED");
}

BOOL     _CMENUITEM_CHECKED_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.CHECKED");
    return FALSE;
}

VOID _CMENUITEM_TOGGLE (CMenuItem *THIS, BOOL     B)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.toggle");
}

BOOL     _CMENUITEM_TOGGLE_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.toggle");
    return FALSE;
}

VOID _CMENUITEM_ENABLED (CMenuItem *THIS, BOOL     B)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.enabled");
}

BOOL     _CMENUITEM_ENABLED_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.enabled");
    return FALSE;
}

VOID _CMENUITEM_COMMAND (CMenuItem *THIS, BYTE     B)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.Command");
}

BYTE     _CMENUITEM_COMMAND_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.Command");
    return 0;
}

VOID _CMENUITEM_MUTUALEXCLUDE (CMenuItem *THIS, LONG     B)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.MutualExclude");
}

LONG     _CMENUITEM_MUTUALEXCLUDE_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.MutualExclude");
    return 0;
}

VOID _CMENUITEM_HIGHFLAGS (CMenuItem *THIS, UWORD    B)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.HIGHFLAGS");
}

UWORD    _CMENUITEM_HIGHFLAGS_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.HIGHFLAGS");
    return 0;
}

VOID _CMENUITEM_ADDSUBITEM (CMenuItem *THIS, CMenuItem *SubItem)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.addSubItem");
}

VOID _CMENUITEM_REMOVESUBITEMS (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.removeSubItems");
}

VOID _CMENUITEM_NEXTITEM (CMenuItem *THIS, CMenuItem *i)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.NextItem");
}

CMenuItem *_CMENUITEM_NEXTITEM_ (CMenuItem *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenuItem.NextItem");
    return NULL;
}

static intptr_t _CMenuItem_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CMENUITEM___init (CMenuItem *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CMenuItem_vtable;
}

