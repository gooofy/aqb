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

static struct Gadget *_gtslider_deploy_cb (GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta)
{
    GTSLIDER_t *slider = (GTSLIDER_t *)gtg;

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

void _GTSLIDER_CONSTRUCTOR (GTSLIDER_t *this, CONST_STRPTR txt,
                            SHORT min, SHORT max, SHORT level, ULONG freedom,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore)
{
    DPRINTF("_GTSLIDER_CONSTRUCTOR: this=0x%08lx, x1=%d, y1=%d, x2=%d, y2=%d\n", this, x1, y1, x2, y2);
    _GTGADGET_CONSTRUCTOR (&this->gadget, txt, s1, x1, y1, s2, x2, y2, user_data, flags, underscore);
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

BOOL _GTSLIDER_disabled_ (GTSLIDER_t *this)
{
    return this->disabled;
}
void _GTSLIDER_disabled (GTSLIDER_t *this, BOOL disabled)
{
    if (_GTGADGET_deployed_ (&this->gadget))
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GA_Disabled, disabled, TAG_DONE);
    this->disabled = disabled;
}

SHORT _GTSLIDER_min_ (GTSLIDER_t *this)
{
    return this->min;
}
void _GTSLIDER_min (GTSLIDER_t *this, SHORT min)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSL_Min, min, TAG_DONE);
    }
    this->min = min;
}

SHORT _GTSLIDER_max_ (GTSLIDER_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSL_Max, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->max;
}
void _GTSLIDER_max (GTSLIDER_t *this, SHORT max)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSL_Max, max, TAG_DONE);
    }
    this->max = max;
}

SHORT _GTSLIDER_level_ (GTSLIDER_t *this)
{
    if (_GTGADGET_deployed_ (&this->gadget) && (GadToolsBase->lib_Version>=39))
    {
        ULONG u;
        LONG n = GT_GetGadgetAttrs(this->gadget.gad, this->gadget.win, NULL, GTSL_Level, (intptr_t)&u, TAG_DONE);
        if (n==1)
            return u;
    }
    return this->level;
}
void _GTSLIDER_level (GTSLIDER_t *this, SHORT level)
{
    if (_GTGADGET_deployed_ (&this->gadget))
    {
        GT_SetGadgetAttrs (this->gadget.gad, this->gadget.win, NULL, GTSL_Level, level, TAG_DONE);
    }
    this->level = level;
}

SHORT _GTSLIDER_maxLevelLen_ (GTSLIDER_t *this)
{
    return this->maxLevelLen;
}
void _GTSLIDER_maxLevelLen  (GTSLIDER_t *this, SHORT i)
{
    this->maxLevelLen = i;
}

CONST_STRPTR  _GTSLIDER_levelFormat_ (GTSLIDER_t *this)
{
    return this->levelFormat;
}
void _GTSLIDER_levelFormat  (GTSLIDER_t *this, CONST_STRPTR s)
{
    this->levelFormat = s;
}

ULONG _GTSLIDER_levelPlace_ (GTSLIDER_t *this)
{
    return this->levelPlace;
}
void  _GTSLIDER_levelPlace  (GTSLIDER_t *this, ULONG u)
{
    this->levelPlace = u;
}

BOOL _GTSLIDER_immediate_ (GTSLIDER_t *this)
{
    return this->immediate;
}
void _GTSLIDER_immediate  (GTSLIDER_t *this, BOOL b)
{
    this->immediate = b;
}

BOOL _GTSLIDER_relVerify_ (GTSLIDER_t *this)
{
    return this->relVerify;
}
void _GTSLIDER_relVerify  (GTSLIDER_t *this, BOOL b)
{
    this->relVerify = b;
}

//GTSL_MaxLevelLen (UWORD)
//GTSL_LevelPlace (ULONG)
//GA_Immediate (BOOL)
//GA_RelVerify (BOOL)
//PGA_Freedom (ULONG)



