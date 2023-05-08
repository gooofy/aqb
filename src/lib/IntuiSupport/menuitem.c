//#define ENABLE_DPRINTF

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

VOID _CMENUITEM_CONSTRUCTOR (CMenuItem *THIS, CMenu *parent, intptr_t userData)
{
    THIS->_cb                       = NULL;
    THIS->_userData                 = userData;
    THIS->_parent                   = parent;
    THIS->_item._item.NextItem      = NULL;
    THIS->_item._item.Flags         = ITEMENABLED | HIGHCOMP ;
    THIS->_item._item.MutualExclude = 0;
    THIS->_item._item.ItemFill      = NULL;
    THIS->_item._item.SelectFill    = NULL;
    THIS->_item._item.Command       = 0;
    THIS->_item._item.SubItem       = NULL;
    THIS->_item._wrapper            = THIS;
    THIS->_subItem                  = NULL;
    THIS->_nextItem                 = NULL;

    if (parent)
        _CMENU_ADDITEM (parent, THIS);
}

VOID _CMENUITEM_CHECKIT (CMenuItem *THIS, BOOL b)
{
    intuis_win_ext_t *ext = _IntuiSupport_get_ext(THIS->_parent->_win_id);
    USHORT cw = ext->draw_info->dri_CheckMark->Width;

    struct MenuItem *item = &THIS->_item._item;
    if (b)
    {
        item->Flags |= CHECKIT;

        if (item->ItemFill)
        {
            DPRINTF ("_CMENUITEM_CHECKIT: moving CHECKIT item %d pixels to the right\n", cw);

            if (item->Flags & ITEMTEXT)
            {
                struct IntuiText *itext = (struct IntuiText *)item->ItemFill;
                itext->LeftEdge += cw;
                if (item->SelectFill)
                {
                    itext = (struct IntuiText *)item->SelectFill;
                    itext->LeftEdge += cw;
                }
            }
            else
            {
                struct Image *img = (struct Image *)item->ItemFill;
                img->LeftEdge += cw;
                if (item->SelectFill)
                {
                    img = (struct Image *)item->SelectFill;
                    img->LeftEdge += cw;
                }
            }
        }
    }
    else
    {
        THIS->_item._item.Flags &= ~CHECKIT;
    }
}

BOOL _CMENUITEM_CHECKIT_ (CMenuItem *THIS)
{
    return THIS->_item._item.Flags & CHECKIT;
}

VOID _CMENUITEM_CHECKED (CMenuItem *THIS, BOOL b)
{
    if (b)
        THIS->_item._item.Flags |= CHECKED;
    else
        THIS->_item._item.Flags &= ~CHECKED;
}

BOOL _CMENUITEM_CHECKED_ (CMenuItem *THIS)
{
    return THIS->_item._item.Flags & CHECKED;
}

VOID _CMENUITEM_TOGGLE (CMenuItem *THIS, BOOL b)
{
    if (b)
        THIS->_item._item.Flags |= MENUTOGGLE;
    else
        THIS->_item._item.Flags &= ~MENUTOGGLE;
}

BOOL _CMENUITEM_TOGGLE_ (CMenuItem *THIS)
{
    return THIS->_item._item.Flags & MENUTOGGLE;
}

VOID _CMENUITEM_ENABLED (CMenuItem *THIS, BOOL enabled)
{
    if (enabled)
        THIS->_item._item.Flags |= ITEMENABLED;
    else
        THIS->_item._item.Flags &= ~ITEMENABLED;
}

BOOL _CMENUITEM_ENABLED_ (CMenuItem *THIS)
{
    return THIS->_item._item.Flags & ITEMENABLED;
}

VOID _CMENUITEM_COMMAND (CMenuItem *THIS, BYTE command)
{
    THIS->_item._item.Command = command;
    if (command)
        THIS->_item._item.Flags |= COMMSEQ;
    else
        THIS->_item._item.Flags &= ~COMMSEQ;
}

BYTE _CMENUITEM_COMMAND_ (CMenuItem *THIS)
{
    return THIS->_item._item.Command;
}

VOID _CMENUITEM_MUTUALEXCLUDE (CMenuItem *THIS, LONG mx)
{
    THIS->_item._item.MutualExclude = mx;
}

LONG _CMENUITEM_MUTUALEXCLUDE_ (CMenuItem *THIS)
{
    return THIS->_item._item.MutualExclude;
}

VOID _CMENUITEM_HIGHFLAGS (CMenuItem *THIS, UWORD flags)
{
    THIS->_item._item.Flags &= ~HIGHFLAGS;
    THIS->_item._item.Flags |= flags & HIGHFLAGS;
}

UWORD _CMENUITEM_HIGHFLAGS_ (CMenuItem *THIS)
{
    return THIS->_item._item.Flags & HIGHFLAGS;
}

VOID _CMENUITEM_ADDSUBITEM (CMenuItem *THIS, CMenuItem *subItem)
{
    CMenuItem *last = THIS->_subItem;
    while (last && last->_nextItem)
        last = last->_nextItem;
    if (!last)
    {
        THIS->_subItem = subItem;
        THIS->_item._item.SubItem = &subItem->_item._item;

        // add » Symbol as second IntuiText
        if (THIS->_item._item.Flags & ITEMTEXT)
        {
            struct IntuiText *itext = (struct IntuiText *) THIS->_item._item.ItemFill;
            if (itext)
            {
                intuis_win_ext_t *ext = _IntuiSupport_get_ext(THIS->_parent->_win_id);
                struct IntuiText *at = (struct IntuiText *) ALLOCATE_(sizeof(*at), 0);
                if (!at)
                    ERROR (ERR_OUT_OF_MEMORY);
                at->FrontPen  = ext->draw_info->dri_Pens[BARDETAILPEN];
                at->BackPen   = 0;
                at->DrawMode  = JAM1;
                at->LeftEdge  = 0;
                at->TopEdge   = 1;
                at->ITextFont = NULL;
                at->IText     = (STRPTR) "»";
                at->NextText  = NULL;
                itext->NextText = at;
            }
        }
    }
    else
    {
        last->_nextItem = subItem;
        last->_item._item.NextItem = &subItem->_item._item;
    }
    subItem->_item._item.NextItem = NULL;
    subItem->_nextItem = NULL;
    subItem->_parent = THIS->_parent;
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

static struct TextFont *_itext_setfont (struct RastPort *rp, const struct TextAttr *tattr)
{
    if (!tattr)
        return NULL;

    struct TextFont *font = OpenFont(tattr);
    if (!font)
        return NULL;

    SetFont (rp, font);
    SetSoftStyle (rp, tattr->ta_Style, 0xff);

    return font;
}

static void _itext_bbox (struct IntuiText *itext, WORD *x1, WORD *y1, WORD *x2, WORD *y2)
{
    struct TextExtent te;
    struct RastPort rp;
    InitRastPort (&rp);

    *x1 = 32000;
    *y1 = 32000;
    *x2 = -32000;
    *y2 = -32000;

    while (itext)
    {
        struct TextFont *font = _itext_setfont (&rp, itext->ITextFont);

        TextExtent (&rp, itext->IText, LEN_(itext->IText), &te);

        if (font)
            CloseFont(font);

        DPRINTF ("_itext_bbox: extent of 0x%08lx (%s): %dx%d\n", (intptr_t)itext, itext->IText, te.te_Width, te.te_Height);

        WORD tx1 = itext->LeftEdge;
        WORD ty1 = itext->TopEdge;

        WORD tx2 = tx1+te.te_Width  - 1;
        WORD ty2 = ty1+te.te_Height - 1;

        if (tx1<*x1)
            *x1 = tx1;
        if (ty1<*y1)
            *y1 = ty1;
        if (tx2>*x2)
            *x2 = tx2;
        if (ty2>*y2)
            *y2 = ty2;

        DPRINTF ("_itext_bbox: itext->NextText=0x%08lx\n", (intptr_t)itext->NextText);
        itext = itext->NextText;
    }
    DPRINTF ("_itext_bbox done.\n");
}

static void _img_bbox (struct Image *img, WORD *x1, WORD *y1, WORD *x2, WORD *y2)
{
    *x1 = 32000;
    *y1 = 32000;
    *x2 = -32000;
    *y2 = -32000;

    while (img)
    {
        DPRINTF ("_img_bbox: extent of 0x%08lx: %dx%d\n", (intptr_t)img, img->Width, img->Height);

        WORD tx1 = img->LeftEdge;
        WORD ty1 = img->TopEdge;

        WORD tx2 = tx1 + img->Width  - 1;
        WORD ty2 = ty1 + img->Height + 2 - 1;

        if (tx1<*x1)
            *x1 = tx1;
        if (ty1<*y1)
            *y1 = ty1;
        if (tx2>*x2)
            *x2 = tx2;
        if (ty2>*y2)
            *y2 = ty2;

        DPRINTF ("_img_bbox: img->NextImage=0x%08lx\n", (intptr_t)img->NextImage);
        img = img->NextImage;
    }
    DPRINTF ("_img_bbox done.\n");
}

VOID _CMENUITEM_BBOX (CMenuItem *THIS, WORD *x1, WORD *y1, WORD *x2, WORD *y2)
{
    *x1 = 32000;
    *y1 = 32000;
    *x2 = -32000;
    *y2 = -32000;

    if (THIS->_item._item.Flags & ITEMTEXT)
    {
        struct IntuiText *itext = (struct IntuiText *) THIS->_item._item.ItemFill;

        DPRINTF ("_CMENUITEM_BBOX: THIS=0x%08lx, itext=0x%08lx, SelectFill=0x%08lx\n",
                 THIS, itext, THIS->_item._item.SelectFill);

        WORD tx1, ty1, tx2, ty2;
        _itext_bbox (itext, &tx1, &ty1, &tx2, &ty2);
        *x1 = tx1; *y1=ty1; *x2=tx2; *y2=ty2;

        itext = (struct IntuiText *) THIS->_item._item.SelectFill;
        if (itext)
        {
            _itext_bbox (itext, &tx1, &ty1, &tx2, &ty2);
            if (tx1<*x1)
                *x1 = tx1;
            if (ty1<*y1)
                *y1 = ty1;
            if (tx2>*x2)
                *x2 = tx2;
            if (ty2>*y2)
                *y2 = ty2;
        }
    }
    else
    {
        struct Image *img = (struct Image *) THIS->_item._item.ItemFill;

        DPRINTF ("_CMENUITEM_BBOX: THIS=0x%08lx, img=0x%08lx, SelectFill=0x%08lx\n",
                 THIS, img, THIS->_item._item.SelectFill);

        WORD tx1, ty1, tx2, ty2;
        _img_bbox (img, &tx1, &ty1, &tx2, &ty2);
        *x1 = tx1; *y1=ty1; *x2=tx2; *y2=ty2;

        img = (struct Image *) THIS->_item._item.SelectFill;
        if (img)
        {
            _img_bbox (img, &tx1, &ty1, &tx2, &ty2);
            if (tx1<*x1)
                *x1 = tx1;
            if (ty1<*y1)
                *y1 = ty1;
            if (tx2>*x2)
                *x2 = tx2;
            if (ty2>*y2)
                *y2 = ty2;
        }
    }
}


static intptr_t _CMenuItem_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_,
};

void _CMENUITEM___init (CMenuItem *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CMenuItem_vtable;
}

