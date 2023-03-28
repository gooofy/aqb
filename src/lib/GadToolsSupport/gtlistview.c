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

static struct Gadget *_gtlistview_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTLISTVIEW_t *gt = (GTLISTVIEW_t *)gtg;

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (LISTVIEW_KIND, gad, &gtg->ng,
                             GT_Underscore    , gtg->underscore,
                             GA_Disabled      , gt->disabled,
                             GTLV_MakeVisible , gt->makeVisible,
                             GTLV_Labels      , (intptr_t) gt->labels,
                             GTLV_ReadOnly    , gt->readOnly,
                             GTLV_ScrollWidth , gt->scrollWidth,
                             GTLV_Selected    , gt->selected,
                             GTLV_ShowSelected, NULL,
                             LAYOUTA_Spacing  , gt->spacing,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtlistview_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = LISTVIEWIDCMP;

    DPRINTF("_gtlistview_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _GTLISTVIEW_CONSTRUCTOR (GTLISTVIEW_t *this,
                            CONST_STRPTR label, struct List * labels,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTLISTVIEW_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _GTGADGET_CONSTRUCTOR (&this->gadget, label, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtlistview_deploy_cb;
    this->labels          = labels;
    this->disabled        = FALSE;
    this->makeVisible     = 0;
    this->readOnly        = FALSE;
    this->scrollWidth     = 16;
    this->selected        = ~0;
    this->spacing         = 0;
}

BOOL _GTLISTVIEW_disabled_ (GTLISTVIEW_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GA_Disabled, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->disabled;
}
void _GTLISTVIEW_disabled (GTLISTVIEW_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    }
    this->disabled = disabled;
}

SHORT _GTLISTVIEW_makeVisible_ (GTLISTVIEW_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTLV_MakeVisible, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->makeVisible;
}
void _GTLISTVIEW_makeVisible (GTLISTVIEW_t *this, SHORT makeVisible)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTLV_MakeVisible, makeVisible, TAG_DONE);
    }
    this->makeVisible = makeVisible;
}

struct List * _GTLISTVIEW_labels_ (GTLISTVIEW_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTLV_Labels, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return (struct List *) u;
    }
    return this->labels;
}
void _GTLISTVIEW_labels (GTLISTVIEW_t *this, struct List * labels)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTLV_Labels, (intptr_t) labels, TAG_DONE);
    }
    this->labels = labels;
}

BOOL _GTLISTVIEW_readOnly_ (GTLISTVIEW_t *this)
{
    return this->readOnly;
}
void _GTLISTVIEW_readOnly (GTLISTVIEW_t *this, BOOL readOnly)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTLV_ReadOnly, readOnly, TAG_DONE);
    }
    this->readOnly = readOnly;
}

USHORT _GTLISTVIEW_scrollWidth_ (GTLISTVIEW_t *this)
{
    return this->scrollWidth;
}
void _GTLISTVIEW_scrollWidth (GTLISTVIEW_t *this, USHORT scrollWidth)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTLV_ScrollWidth, scrollWidth, TAG_DONE);
    }
    this->scrollWidth = scrollWidth;
}

USHORT _GTLISTVIEW_selected_ (GTLISTVIEW_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=36))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTLV_Selected, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->selected;
}
void _GTLISTVIEW_selected (GTLISTVIEW_t *this, USHORT selected)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTLV_Selected, selected, TAG_DONE);
    }
    this->selected = selected;
}

USHORT _GTLISTVIEW_spacing_ (GTLISTVIEW_t *this)
{
    return this->spacing;
}
void _GTLISTVIEW_spacing (GTLISTVIEW_t *this, USHORT spacing)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, LAYOUTA_Spacing, spacing, TAG_DONE);
    }
    this->spacing = spacing;
}
