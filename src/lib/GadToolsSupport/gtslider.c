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

static struct Gadget *_gtslider_deploy_cb (CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    CGTSlider *slider = (CGTSlider *)gtg;

    DPRINTF("_gtslider_deploy_cb: slider=0x%08lx, min=%d, max=%d, level=%d\n", slider, slider->min, slider->max, slider->level);

    gtg->ng.ng_VisualInfo = vinfo;
    gtg->ng.ng_TextAttr   = ta;

    gtg->gad = CreateGadget (SLIDER_KIND, gad, &gtg->ng,
                             GA_Disabled     , slider->disabled,
                             GT_Underscore   , gtg->underscore,
                             GTSL_Min        , slider->min,
                             GTSL_Max        , slider->max,
                             GTSL_Level      , slider->level,
                             PGA_Freedom     , slider->freedom,
                             GTSL_MaxLevelLen, slider->maxLevelLen,
                             GTSL_LevelFormat, (intptr_t) slider->levelFormat,
                             GTSL_LevelPlace , slider->levelPlace,
                             GA_Immediate    , slider->immediate,
                             GA_RelVerify    , slider->relVerify,
                             TAG_DONE);

    if (!gtg->gad)
    {
        DPRINTF ("_gtslider_deploy_cb: CreateGadget() failed.\n");
        ERROR(AE_GTG_CREATE);
        return gad;
    }

    // take care of IDCMP flags
    ULONG gidcmp = SLIDERIDCMP;

    DPRINTF("_gtslider_deploy_cb: gtg->win->IDCMPFlags=0x%08lx, gidcmp=0x%08lx\n", gtg->win->IDCMPFlags, gidcmp);

    if (gidcmp && ( (gtg->win->IDCMPFlags & gidcmp) != gidcmp ) )
        ModifyIDCMP (gtg->win, gtg->win->IDCMPFlags | gidcmp);

    return gtg->gad;
}

void _CGTSLIDER_CONSTRUCTOR (CGTSlider *this, CONST_STRPTR txt,
                            SHORT min, SHORT max, SHORT level, ULONG freedom,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_CGTSlider_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _CGTGADGET_CONSTRUCTOR (&this->gadget, txt, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
    this->gadget.deploy_cb = _gtslider_deploy_cb;
    this->disabled         = FALSE;
    this->min              = min;
    this->max              = max;
    this->level            = level;
    this->freedom          = freedom;
    this->maxLevelLen      = 6;
    this->levelFormat      = (CONST_STRPTR)"%d";
    this->levelPlace       = PLACETEXT_LEFT;
    this->immediate        = FALSE;
    this->relVerify        = FALSE;
}

BOOL _CGTSLIDER_DISABLED_ (CGTSlider *this)
{
    return this->disabled;
}
void _CGTSLIDER_DISABLED (CGTSlider *this, BOOL disabled)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    this->disabled = disabled;
}

SHORT _CGTSLIDER_MIN_ (CGTSlider *this)
{
    return this->min;
}
void _CGTSLIDER_MIN (CGTSlider *this, SHORT min)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSL_Min, min, TAG_DONE);
    }
    this->min = min;
}

SHORT _CGTSLIDER_MAX_ (CGTSlider *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSL_Max, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->max;
}
void _CGTSLIDER_MAX (CGTSlider *this, SHORT max)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSL_Max, max, TAG_DONE);
    }
    this->max = max;
}

SHORT _CGTSLIDER_LEVEL_ (CGTSlider *this)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSL_Level, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->level;
}
void _CGTSLIDER_LEVEL (CGTSlider *this, SHORT level)
{
    if (_CGTGADGET_DEPLOYED_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSL_Level, level, TAG_DONE);
    }
    this->level = level;
}

SHORT _CGTSLIDER_MAXLEVELLEN_ (CGTSlider *this)
{
    return this->maxLevelLen;
}
void _CGTSLIDER_MAXLEVELLEN  (CGTSlider *this, SHORT i)
{
    this->maxLevelLen = i;
}

CONST_STRPTR  _CGTSLIDER_LEVELFORMAT_ (CGTSlider *this)
{
    return this->levelFormat;
}
void _CGTSLIDER_LEVELFORMAT  (CGTSlider *this, CONST_STRPTR s)
{
    this->levelFormat = s;
}

ULONG _CGTSLIDER_LEVELPLACE_ (CGTSlider *this)
{
    return this->levelPlace;
}
void  _CGTSLIDER_LEVELPLACE  (CGTSlider *this, ULONG u)
{
    this->levelPlace = u;
}

BOOL _CGTSLIDER_IMMEDIATE_ (CGTSlider *this)
{
    return this->immediate;
}
void _CGTSLIDER_IMMEDIATE  (CGTSlider *this, BOOL b)
{
    this->immediate = b;
}

BOOL _CGTSLIDER_RELVERIFY_ (CGTSlider *this)
{
    return this->relVerify;
}
void _CGTSLIDER_RELVERIFY  (CGTSlider *this, BOOL b)
{
    this->relVerify = b;
}

//GTSL_MaxLevelLen (UWORD)
//GTSL_LevelPlace (ULONG)
//GA_Immediate (BOOL)
//GA_RelVerify (BOOL)
//PGA_Freedom (ULONG)

static intptr_t _CGTSlider_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_
};


void _CGTSLIDER___init (CGTSlider *THIS)
{
    THIS->gadget._vTablePtr = (intptr_t **) &_CGTSlider_vtable;
}


