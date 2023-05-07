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

VOID _CMENU_CONSTRUCTOR (CMenu *THIS, STRPTR Name, CMenu *prevMenu)
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

    if (prevMenu)
    {
        prevMenu->_nextMenu = THIS;
        prevMenu->_menu.NextMenu = &THIS->_menu;
    }
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
    item->_item._item.NextItem = NULL;
    if (THIS->_lastItem)
    {
        THIS->_lastItem->_item._item.NextItem = &item->_item._item;
        THIS->_lastItem = THIS->_lastItem->_nextItem = item;
    }
    else
    {
        THIS->_menu.FirstItem = &item->_item._item;
        THIS->_firstItem = THIS->_lastItem = item;
    }
}

VOID _CMENU_REMOVEALLITEMS (CMenu *THIS)
{
    THIS->_firstItem = NULL;
    THIS->_lastItem = NULL;
}

#define ITEM_MIN_HEIGHT 8

static void _layoutItems (CMenuItem *cfirstitem)
{
    SHORT y = 0;
    UWORD maxw = 0;
    for (CMenuItem *citem=cfirstitem; citem; citem=citem->_nextItem)
    {
        SHORT ix1, iy1, ix2, iy2;
        _CMENUITEM_BBOX (citem, &ix1, &iy1, &ix2, &iy2);

        DPRINTF ("_layoutItems: bbox: %d/%d - %d/%d\n", ix1,iy1, ix2,iy2);

        SHORT iw = ix2-ix1+1;
        SHORT ih = iy2-iy1+1;
        DPRINTF ("_layoutItems: ->iw=%d, ih=%d\n", iw, ih);

        if (ih<ITEM_MIN_HEIGHT)
            ih = ITEM_MIN_HEIGHT;

        citem->_item._item.LeftEdge = 0;
        citem->_item._item.TopEdge  = y;
        citem->_item._item.Width    = ix1+iw;
        citem->_item._item.Height   = iy1+ih+1;

        DPRINTF ("_layoutItems: ->item at %d/%d, %dx%d\n",
                 citem->_item._item.LeftEdge, citem->_item._item.TopEdge, citem->_item._item.Width, citem->_item._item.Height);
        y += iy1+ih+1;
        if (citem->_item._item.Width>maxw)
            maxw = citem->_item._item.Width;
    }

    // make all items the same (full) width
    for (CMenuItem *citem=cfirstitem; citem; citem=citem->_nextItem)
        citem->_item._item.Width    = maxw;

}

static BOOL _menu_msg_cb (SHORT wid, struct Window *win, struct IntuiMessage *msg, window_refresh_cb_t refresh_cb, void *refresh_ud)
{
    //DPRINTF ("_menu_msg_cb: msg->Class=0x%08lx\n", msg->Class);
    if (msg->Class == IDCMP_MENUPICK)
    {
        intuis_win_ext_t *ext        = _IntuiSupport_get_ext(wid+1);
        UWORD             menuNumber = msg->Code;

        //DPRINTF ("_menu_msg_cb: IDCMP_MENUPICK, wid=%d, menuNumber=%d\n", wid, menuNumber);

        //DPRINTF ("_menu_msg_cb: IDCMP_MENUPICK, wid=%d, menuNum=%d, itemNum=%d, subNum=%d\n",
        //         wid, MENUNUM(menuNumber), ITEMNUM(menuNumber), SUBNUM(menuNumber));

        MenuItemUD *item = (MenuItemUD*) ItemAddress(&ext->deployedMenu->_menu, menuNumber);

        //DPRINTF ("_menu_msg_cb: IDCMP_MENUPICK, item=0x%08lx\n", item);

        CMenuItem *citem = item->_wrapper;
        //DPRINTF ("_menu_msg_cb: IDCMP_MENUPICK, citem=0x%08lx, cb=0x%08lx\n", citem, citem->_cb);

        if (citem->_cb)
            citem->_cb(citem);

        return TRUE;
    }
    return FALSE;
}

VOID _CMENU_DEPLOY (CMenu *THIS)
{
    intuis_win_ext_t *ext = _IntuiSupport_get_ext(THIS->_win_id);

    // layout time

    // scratch rastport for text/font measurements
    struct RastPort  tmprp;
    InitRastPort(&tmprp);
    SetFont (&tmprp, ext->screen_font);

    UWORD x = 2;

    for (CMenu *cmenu=THIS; cmenu; cmenu=cmenu->_nextMenu)
    {
        struct Menu *menu = &cmenu->_menu;


        menu->TopEdge  = 0;
        menu->Height   = ext->screen_font->tf_YSize;
        menu->LeftEdge = x;
        menu->Width    = TextLength (&tmprp, menu->MenuName, LEN_ (menu->MenuName))
                         + 2 * (ext->screen->BarHBorder - ext->screen->BarVBorder);

        _layoutItems (cmenu->_firstItem);

        x += menu->Width + ext->screen_font->tf_XSize;
    }

    struct Window *win = _aqb_get_win (THIS->_win_id);
    SetMenuStrip (win, &THIS->_menu);

    DPRINTF ("CMENU_DEPLOY: before IDCMPFlags=0x%08lx\n", win->IDCMPFlags);
    ModifyIDCMP (win, win->IDCMPFlags | IDCMP_MENUPICK);
    DPRINTF ("CMENU_DEPLOY: after  IDCMPFlags=0x%08lx\n", win->IDCMPFlags);

    if (!ext->menu_msg_cb_installed)
    {
        DPRINTF ("CMENU_DEPLOY: installing custom msg callback for wid #%d\n", _g_cur_win_id);
                _window_add_msg_cb (_g_cur_win_id, _menu_msg_cb);
        ext->menu_msg_cb_installed = TRUE;
    }
    ext->deployedMenu = THIS;
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


