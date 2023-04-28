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

static struct Gadget *_gtpalette_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTPalette *gt = (CGTPalette *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (PALETTE_KIND, gad, &gtg->ng,
                             GT_Underscore   , gtg->underscore,
                             GA_Disabled     , gt->disabled,
                             GTPA_Depth      , gt->depth,
                             GTPA_Color      , gt->color,
                             GTPA_ColorOffset , gt->colorOffset,
                             GTPA_IndicatorWidth , gt->indicatorWidth,
                             GTPA_IndicatorHeight , gt->indicatorHeight,
                             GTPA_ColorTable , (intptr_t) gt->colorTable,
                             GTPA_NumColors  , gt->numColors,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtpalette_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = PALETTEIDCMP;

    DPRINTF("_gtpalette_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _CGTPalette_CONSTRUCTOR (CGTPalette *this,
                            CONST_STRPTR label, USHORT numColors,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTPalette_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _CGTGadget_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtpalette_deploy_cb;
    this->disabled        = FALSE;
    this->color           = 1;
    this->colorOffset     = 0;
    this->indicatorWidth  = 0;
    this->indicatorHeight = 0;
    this->colorTable      = NULL;
    this->numColors       = numColors;
}

BOOL _CGTPalette_disabled_ (CGTPalette *this)
{
    if (_CGTGadget_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GA_Disabled, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->disabled;
}
void _CGTPalette_disabled (CGTPalette *this, BOOL disabled)
{
    if (_CGTGadget_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
}

USHORT _CGTPalette_depth_ (CGTPalette *this)
{
    return this->depth;
}
void _CGTPalette_depth (CGTPalette *this, USHORT depth)
{
    if (_CGTGadget_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTPA_Depth, depth, TAG_DONE);
    }
    this->depth = depth;
}

UBYTE _CGTPalette_color_ (CGTPalette *this)
{
    if (_CGTGadget_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTPA_Color, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->color;
}
void _CGTPalette_color (CGTPalette *this, UBYTE color)
{
    if (_CGTGadget_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTPA_Color, color, TAG_DONE);
    }
    this->color = color;
}

UBYTE _CGTPalette_colorOffset_ (CGTPalette *this)
{
    if (_CGTGadget_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTPA_ColorOffset, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->colorOffset;
}
void _CGTPalette_colorOffset (CGTPalette *this, UBYTE colorOffset)
{
    if (_CGTGadget_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTPA_ColorOffset, colorOffset, TAG_DONE);
    }
    this->colorOffset = colorOffset;
}

USHORT _CGTPalette_indicatorWidth_ (CGTPalette *this)
{
    return this->indicatorWidth;
}
void _CGTPalette_indicatorWidth (CGTPalette *this, USHORT indicatorWidth)
{
    if (_CGTGadget_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTPA_IndicatorWidth, indicatorWidth, TAG_DONE);
    }
    this->indicatorWidth = indicatorWidth;
}

USHORT _CGTPalette_indicatorHeight_ (CGTPalette *this)
{
    return this->indicatorHeight;
}
void _CGTPalette_indicatorHeight (CGTPalette *this, USHORT indicatorHeight)
{
    if (_CGTGadget_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTPA_IndicatorHeight, indicatorHeight, TAG_DONE);
    }
    this->indicatorHeight = indicatorHeight;
}

UBYTE * _CGTPalette_colorTable_ (CGTPalette *this)
{
    if (_CGTGadget_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTPA_ColorTable, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return (UBYTE *)u;
    }
    return this->colorTable;
}
void _CGTPalette_colorTable (CGTPalette *this, UBYTE * colorTable)
{
    if (_CGTGadget_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTPA_ColorTable, (intptr_t)colorTable, TAG_DONE);
    }
    this->colorTable = colorTable;
}

USHORT _CGTPalette_numColors_ (CGTPalette *this)
{
    return this->numColors;
}
void _CGTPalette_numColors (CGTPalette *this, USHORT numColors)
{
    if (_CGTGadget_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTPA_NumColors, numColors, TAG_DONE);
    }
    this->numColors = numColors;
}

static intptr_t _CGTPalette_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_
};

void _CGTPalette___init (CGTPalette *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTPalette_vtable;
}

