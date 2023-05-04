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

VOID _CMENU_CONSTRUCTOR (CMenu *THIS, STRPTR Name)
{
    _aqb_get_output (/*needGfx=*/TRUE);

    THIS->_firstItem      = NULL;
    THIS->_lastItem       = NULL;
    THIS->_nextMenu       = NULL;
    THIS->_win_id         = _g_cur_win_id;

    THIS->_menu.NextMenu  = NULL;
    THIS->_menu.Flags     = MENUENABLED;
    THIS->_menu.MenuName  = Name;
    THIS->_menu.FirstItem = NULL;
}

VOID _CMENU_NEXTMENU (CMenu *THIS, CMenu *x)
{
    THIS->_menu.NextMenu  = &x->_menu;
}

CMenu *_CMENU_NEXTMENU_ (CMenu *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenu.NextMenu");
    return NULL;
}

VOID _CMENU_FIRSTITEM (CMenu *THIS, CMenuItem *i)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenu.FirstItem");
}

CMenuItem *_CMENU_FIRSTITEM_ (CMenu *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenu.FirstItem");
    return NULL;
}

VOID _CMENU_ENABLED (CMenu *THIS, BOOL     B)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenu.enabled");
}

BOOL     _CMENU_ENABLED_ (CMenu *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenu.enabled");
    return FALSE;
}

VOID _CMENU_ADDITEM (CMenu *THIS, CMenuItem *item)
{
    item->_item.NextItem = NULL;
    if (THIS->_lastItem)
    {
        THIS->_lastItem->_item.NextItem = &item->_item;
        THIS->_lastItem = THIS->_lastItem->_nextItem = item;
    }
    else
    {
        THIS->_menu.FirstItem = &item->_item;
        THIS->_firstItem = THIS->_lastItem = item;
    }
}

VOID _CMENU_REMOVEALLITEMS (CMenu *THIS)
{
    THIS->_firstItem = NULL;
    THIS->_lastItem = NULL;
}

static void _layoutItems (struct RastPort *tmprp, CMenuItem *citem)
{
    SHORT y=0;
    while (citem)
    {
        citem->_item.LeftEdge=0;
        citem->_item.TopEdge=y;
        citem->_item.Width=100;
        citem->_item.Height=10;

        y += 10;

        citem=citem->_nextItem;
    }
}

VOID _CMENU_DEPLOY (CMenu *THIS)
{
    intuis_win_ext_t *ext = _IntuiSupport_get_ext(THIS->_win_id);

    // layout time

    // scratch rastport for text/font measurements
    struct RastPort  tmprp;
    InitRastPort(&tmprp);

    UWORD x = 2;

    for (CMenu *cmenu=THIS; cmenu; cmenu=cmenu->_nextMenu)
    {
        struct Menu *menu = &cmenu->_menu;

        SetFont (&tmprp, ext->screen_font);

        menu->TopEdge  = 0;
        menu->Height   = ext->screen_font->tf_YSize;
        menu->LeftEdge = x;
        menu->Width    = TextLength (&tmprp, menu->MenuName, LEN_ (menu->MenuName))
                         + 2 * (ext->screen->BarHBorder - ext->screen->BarVBorder);

        _layoutItems (&tmprp, cmenu->_firstItem);


        x += menu->Width + ext->screen_font->tf_XSize;
    }

    struct Window *win = _aqb_get_win (THIS->_win_id);
    SetMenuStrip (win, &THIS->_menu);
}

VOID _CMENU_UNDEPLOY (CMenu *THIS)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: CMenu.undeploy");
}


static intptr_t _CMenu_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};

void _CMENU___init (CMenu *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CMenu_vtable;
}


