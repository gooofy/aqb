#ifndef HAVE_GADTOOLS_SUPPORT_H
#define HAVE_GADTOOLS_SUPPORT_H

#include <exec/types.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#include <libraries/gadtools.h>

#include "../Collections/Collections.h"

#define AE_GTG_CREATE   400
#define AE_GTG_MODIFY   401
#define AE_GTG_DEPLOY   402
#define AE_GTG_SELECTED 403
#define AE_GTG_CALLBACK 404
#define AE_GTG_BUFFER   405
#define AE_GTG_NUM      406

/***********************************************************************************
 *
 * CGTGadget
 *
 ***********************************************************************************/

typedef struct CGTGadget_   CGTGadget;

typedef void (*gtgadget_cb_t)(CGTGadget *gtg, USHORT code);
typedef struct Gadget * (*gtgadget_deploy_cb_t)(CGTGadget *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta);

struct CGTGadget_
{
    intptr_t            **_vTablePtr;

    gtgadget_cb_t         gadgetup_cb;
    gtgadget_cb_t         gadgetdown_cb;
    gtgadget_cb_t         gadgetmove_cb;

    void                 *user_data;
    ULONG                 underscore;

    CGTGadget           *prev, *next;

    struct NewGadget      ng;

    struct Gadget        *gad;
    struct Window        *win;

    gtgadget_deploy_cb_t  deploy_cb;
};

void _CGTGadget_CONSTRUCTOR (CGTGadget *this, CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// CGTGadget properties
SHORT        _CGTGadget_x1_        (CGTGadget *this);
void         _CGTGadget_x1         (CGTGadget *this, SHORT x1);
SHORT        _CGTGadget_y1_        (CGTGadget *this);
void         _CGTGadget_y1         (CGTGadget *this, SHORT y1);
SHORT        _CGTGadget_x2_        (CGTGadget *this);
void         _CGTGadget_x2         (CGTGadget *this, SHORT x2);
SHORT        _CGTGadget_y2_        (CGTGadget *this);
void         _CGTGadget_y2         (CGTGadget *this, SHORT y2);
CONST_STRPTR _CGTGadget_text_      (CGTGadget *this);
void         _CGTGadget_text       (CGTGadget *this, STRPTR text);
SHORT        _CGTGadget_id_        (CGTGadget *this);
void         _CGTGadget_id         (CGTGadget *this, SHORT id);
ULONG        _CGTGadget_flags_     (CGTGadget *this);
void         _CGTGadget_flags      (CGTGadget *this, ULONG flags);
BOOL         _CGTGadget_deployed_  (CGTGadget *this);

/***********************************************************************************
 *
 * CGTButton
 *
 ***********************************************************************************/

typedef struct CGTButton_   CGTButton;
struct CGTButton_
{
    CGTGadget      gadget;
    BOOL            disabled;
    BOOL            immediate;
};

void _CGTButton_CONSTRUCTOR (CGTButton *this, CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// CGTButton properties
BOOL _CGTButton_disabled_ (CGTButton *this);
void _CGTButton_disabled  (CGTButton *this, BOOL disabled);

BOOL _CGTButton_immediate_ (CGTButton *this);
void _CGTButton_immediate  (CGTButton *this, BOOL value);

/***********************************************************************************
 *
 * CGTCheckBox
 *
 ***********************************************************************************/

typedef struct CGTCheckBox_ CGTCheckBox;
struct CGTCheckBox_
{
    CGTGadget      gadget;
    BOOL            disabled;
    BOOL            checked;
    BOOL            scaled;
};

void _CGTCheckBox_CONSTRUCTOR (CGTCheckBox *this, CONST_STRPTR label,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

// CGTCheckBox properties
BOOL _CGTCheckBox_disabled_ (CGTCheckBox *this);
void _CGTCheckBox_disabled  (CGTCheckBox *this, BOOL disabled);
BOOL _CGTCheckBox_CHECKED_  (CGTCheckBox *this);
void _CGTCheckBox_CHECKED   (CGTCheckBox *this, BOOL checked);
BOOL _CGTCheckBox_scaled_   (CGTCheckBox *this);
void _CGTCheckBox_scaled    (CGTCheckBox *this, BOOL scaled);

/***********************************************************************************
 *
 * GTSLIDER
 *
 ***********************************************************************************/

typedef struct GTSLIDER_   GTSLIDER;
struct GTSLIDER_
{
    CGTGadget      gadget;
    BOOL            disabled;
    SHORT           min, max, level;
    ULONG           freedom;
    SHORT           maxLevelLen;
    CONST_STRPTR    levelFormat;
    ULONG           levelPlace;
    BOOL            immediate, relVerify;
};

void _GTSLIDER_CONSTRUCTOR (GTSLIDER *this, CONST_STRPTR label,
                            SHORT min, SHORT max, SHORT level, ULONG orient,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// GTSLIDER properties
BOOL          _GTSLIDER_disabled_ (GTSLIDER *this);
void          _GTSLIDER_disabled  (GTSLIDER *this, BOOL disabled);

SHORT         _GTSLIDER_min_ (GTSLIDER *this);
void          _GTSLIDER_min  (GTSLIDER *this, SHORT i);

SHORT         _GTSLIDER_max_ (GTSLIDER *this);
void          _GTSLIDER_max  (GTSLIDER *this, SHORT i);

SHORT         _GTSLIDER_level_ (GTSLIDER *this);
void          _GTSLIDER_level  (GTSLIDER *this, SHORT i);

SHORT         _GTSLIDER_maxLevelLen_ (GTSLIDER *this);
void          _GTSLIDER_maxLevelLen  (GTSLIDER *this, SHORT i);

CONST_STRPTR  _GTSLIDER_levelFormat_ (GTSLIDER *this);
void          _GTSLIDER_levelFormat  (GTSLIDER *this, CONST_STRPTR s);

ULONG         _GTSLIDER_levelPlace_ (GTSLIDER *this);
void          _GTSLIDER_levelPlace  (GTSLIDER *this, ULONG u);

BOOL          _GTSLIDER_immediate_ (GTSLIDER *this);
void          _GTSLIDER_immediate  (GTSLIDER *this, BOOL b);

BOOL          _GTSLIDER_relVerify_ (GTSLIDER *this);
void          _GTSLIDER_relVerify  (GTSLIDER *this, BOOL b);

ULONG         _GTSLIDER_freedom_ (GTSLIDER *this);
void          _GTSLIDER_freedom  (GTSLIDER *this, ULONG u);

/***********************************************************************************
 *
 * GTTEXT
 *
 ***********************************************************************************/

typedef struct GTTEXT_ GTTEXT;

struct GTTEXT_
{
    CGTGadget      gadget;
    CONST_STRPTR    text;
    BOOL            copyText;
    BOOL            border;
    UBYTE           frontPen;
    UBYTE           backPen;
    UBYTE           justification;
    BOOL            clipped;
};

void _GTTEXT_CONSTRUCTOR (GTTEXT *this, CONST_STRPTR label, CONST_STRPTR text,
                          BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                          void *user_data, ULONG flags, ULONG underscore);

CONST_STRPTR           _GTTEXT_text_ (GTTEXT *this);
void                   _GTTEXT_text  (GTTEXT *this, CONST_STRPTR value);

BOOL                   _GTTEXT_copyText_ (GTTEXT *this);
void                   _GTTEXT_copyText  (GTTEXT *this, BOOL value);

BOOL                   _GTTEXT_border_ (GTTEXT *this);
void                   _GTTEXT_border  (GTTEXT *this, BOOL value);

UBYTE                  _GTTEXT_frontPen_ (GTTEXT *this);
void                   _GTTEXT_frontPen  (GTTEXT *this, UBYTE value);

UBYTE                  _GTTEXT_backPen_ (GTTEXT *this);
void                   _GTTEXT_backPen  (GTTEXT *this, UBYTE value);

UBYTE                  _GTTEXT_justification_ (GTTEXT *this);
void                   _GTTEXT_justification  (GTTEXT *this, UBYTE value);

BOOL                   _GTTEXT_clipped_ (GTTEXT *this);
void                   _GTTEXT_clipped  (GTTEXT *this, BOOL value);

/***********************************************************************************
 *
 * GTSCROLLER
 *
 ***********************************************************************************/

typedef struct GTSCROLLER_ GTSCROLLER;

struct GTSCROLLER_
{
    CGTGadget      gadget;
    BOOL            disabled;
    BOOL            relVerify;
    BOOL            immediate;
    SHORT           top;
    SHORT           total;
    SHORT           visible;
    USHORT          arrows;
    ULONG           freedom;
};

void _GTSCROLLER_CONSTRUCTOR (GTSCROLLER *this, CONST_STRPTR label,
                              SHORT top, SHORT total, SHORT visible, ULONG freedom,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTSCROLLER_disabled_ (GTSCROLLER *this);
void                   _GTSCROLLER_disabled  (GTSCROLLER *this, BOOL value);

BOOL                   _GTSCROLLER_relVerify_ (GTSCROLLER *this);
void                   _GTSCROLLER_relVerify  (GTSCROLLER *this, BOOL value);

BOOL                   _GTSCROLLER_immediate_ (GTSCROLLER *this);
void                   _GTSCROLLER_immediate  (GTSCROLLER *this, BOOL value);

SHORT                  _GTSCROLLER_top_ (GTSCROLLER *this);
void                   _GTSCROLLER_top  (GTSCROLLER *this, SHORT value);

SHORT                  _GTSCROLLER_total_ (GTSCROLLER *this);
void                   _GTSCROLLER_total  (GTSCROLLER *this, SHORT value);

BOOL                   _GTSCROLLER_visible_ (GTSCROLLER *this);
void                   _GTSCROLLER_visible  (GTSCROLLER *this, BOOL value);

USHORT                 _GTSCROLLER_arrows_ (GTSCROLLER *this);
void                   _GTSCROLLER_arrows  (GTSCROLLER *this, USHORT value);

ULONG                  _GTSCROLLER_freedom_ (GTSCROLLER *this);
void                   _GTSCROLLER_freedom  (GTSCROLLER *this, ULONG value);

/***********************************************************************************
 *
 * GTSTRING
 *
 ***********************************************************************************/

typedef struct GTSTRING_ GTSTRING;

struct GTSTRING_
{
    CGTGadget      gadget;
    BOOL            disabled;
    BOOL            immediate;
    BOOL            tabCycle;
    CONST_STRPTR    str;
    USHORT          maxChars;
    BOOL            exitHelp;
    CONST_STRPTR    justification;
    BOOL            replaceMode;
};

void _GTSTRING_CONSTRUCTOR (GTSTRING *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTSTRING_disabled_ (GTSTRING *this);
void                   _GTSTRING_disabled  (GTSTRING *this, BOOL value);

BOOL                   _GTSTRING_immediate_ (GTSTRING *this);
void                   _GTSTRING_immediate  (GTSTRING *this, BOOL value);

BOOL                   _GTSTRING_tabCycle_ (GTSTRING *this);
void                   _GTSTRING_tabCycle  (GTSTRING *this, BOOL value);

CONST_STRPTR           _GTSTRING_str_ (GTSTRING *this);
void                   _GTSTRING_str  (GTSTRING *this, CONST_STRPTR value);

USHORT                 _GTSTRING_maxChars_ (GTSTRING *this);
void                   _GTSTRING_maxChars  (GTSTRING *this, USHORT value);

BOOL                   _GTSTRING_exitHelp_ (GTSTRING *this);
void                   _GTSTRING_exitHelp  (GTSTRING *this, BOOL value);

CONST_STRPTR           _GTSTRING_justification_ (GTSTRING *this);
void                   _GTSTRING_justification  (GTSTRING *this, CONST_STRPTR value);

BOOL                   _GTSTRING_replaceMode_ (GTSTRING *this);
void                   _GTSTRING_replaceMode  (GTSTRING *this, BOOL value);

/***********************************************************************************
 *
 * GTINTEGER
 *
 ***********************************************************************************/

typedef struct GTINTEGER_ GTINTEGER;

struct GTINTEGER_
{
    CGTGadget      gadget;
    BOOL            disabled;
    BOOL            immediate;
    BOOL            tabCycle;
    LONG            number;
    USHORT          maxChars;
    BOOL            exitHelp;
    CONST_STRPTR    justification;
    BOOL            replaceMode;
};

void _GTINTEGER_CONSTRUCTOR (GTINTEGER *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTINTEGER_disabled_ (GTINTEGER *this);
void                   _GTINTEGER_disabled  (GTINTEGER *this, BOOL value);

BOOL                   _GTINTEGER_immediate_ (GTINTEGER *this);
void                   _GTINTEGER_immediate  (GTINTEGER *this, BOOL value);

BOOL                   _GTINTEGER_tabCycle_ (GTINTEGER *this);
void                   _GTINTEGER_tabCycle  (GTINTEGER *this, BOOL value);

LONG                   _GTINTEGER_number_ (GTINTEGER *this);
void                   _GTINTEGER_number  (GTINTEGER *this, LONG value);

USHORT                 _GTINTEGER_maxChars_ (GTINTEGER *this);
void                   _GTINTEGER_maxChars  (GTINTEGER *this, USHORT value);

BOOL                   _GTINTEGER_exitHelp_ (GTINTEGER *this);
void                   _GTINTEGER_exitHelp  (GTINTEGER *this, BOOL value);

CONST_STRPTR           _GTINTEGER_justification_ (GTINTEGER *this);
void                   _GTINTEGER_justification  (GTINTEGER *this, CONST_STRPTR value);

BOOL                   _GTINTEGER_replaceMode_ (GTINTEGER *this);
void                   _GTINTEGER_replaceMode  (GTINTEGER *this, BOOL value);

/***********************************************************************************
 *
 * GTNUMBER
 *
 ***********************************************************************************/

typedef struct GTNUMBER_ GTNUMBER;

struct GTNUMBER_
{
    CGTGadget      gadget;
    LONG            number;
    BOOL            border;
    UBYTE           frontPen;
    UBYTE           backPen;
    UBYTE           justification;
    CONST_STRPTR    format;
    ULONG           maxNumberLen;
    BOOL            clipped;
};

void _GTNUMBER_CONSTRUCTOR (GTNUMBER *this,
                            CONST_STRPTR label, LONG number,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

LONG                   _GTNUMBER_number_ (GTNUMBER *this);
void                   _GTNUMBER_number  (GTNUMBER *this, LONG value);

BOOL                   _GTNUMBER_border_ (GTNUMBER *this);
void                   _GTNUMBER_border  (GTNUMBER *this, BOOL value);

UBYTE                  _GTNUMBER_frontPen_ (GTNUMBER *this);
void                   _GTNUMBER_frontPen  (GTNUMBER *this, UBYTE value);

UBYTE                  _GTNUMBER_backPen_ (GTNUMBER *this);
void                   _GTNUMBER_backPen  (GTNUMBER *this, UBYTE value);

UBYTE                  _GTNUMBER_justification_ (GTNUMBER *this);
void                   _GTNUMBER_justification  (GTNUMBER *this, UBYTE value);

CONST_STRPTR           _GTNUMBER_format_ (GTNUMBER *this);
void                   _GTNUMBER_format  (GTNUMBER *this, CONST_STRPTR value);

ULONG                  _GTNUMBER_maxNumberLen_ (GTNUMBER *this);
void                   _GTNUMBER_maxNumberLen  (GTNUMBER *this, ULONG value);

BOOL                   _GTNUMBER_clipped_ (GTNUMBER *this);
void                   _GTNUMBER_clipped  (GTNUMBER *this, BOOL value);

/***********************************************************************************
 *
 * GTMX
 *
 ***********************************************************************************/

typedef struct GTMX_ GTMX;

struct GTMX_
{
    CGTGadget      gadget;
    BOOL            disabled;
    CONST_STRPTR *  labels;
    USHORT          active;
    USHORT          spacing;
    BOOL            scaled;
    ULONG           titlePlace;
};

void _GTMX_CONSTRUCTOR (GTMX *this,
                            CONST_STRPTR label, CONST_STRPTR * labels, 
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTMX_disabled_ (GTMX *this);
void                   _GTMX_disabled  (GTMX *this, BOOL value);

CONST_STRPTR *         _GTMX_labels_ (GTMX *this);
void                   _GTMX_labels  (GTMX *this, CONST_STRPTR * value);

USHORT                 _GTMX_active_ (GTMX *this);
void                   _GTMX_active  (GTMX *this, USHORT value);

USHORT                 _GTMX_spacing_ (GTMX *this);
void                   _GTMX_spacing  (GTMX *this, USHORT value);

BOOL                   _GTMX_scaled_ (GTMX *this);
void                   _GTMX_scaled  (GTMX *this, BOOL value);

ULONG                  _GTMX_titlePlace_ (GTMX *this);
void                   _GTMX_titlePlace  (GTMX *this, ULONG value);

/***********************************************************************************
 *
 * GTCYCLE
 *
 ***********************************************************************************/

typedef struct GTCYCLE_ GTCYCLE;

struct GTCYCLE_
{
    CGTGadget      gadget;
    BOOL            disabled;
    CONST_STRPTR *  labels;
    USHORT          active;
};

void _GTCYCLE_CONSTRUCTOR (GTCYCLE *this,
                            CONST_STRPTR label, CONST_STRPTR * labels,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTCYCLE_disabled_ (GTCYCLE *this);
void                   _GTCYCLE_disabled  (GTCYCLE *this, BOOL value);

CONST_STRPTR *         _GTCYCLE_labels_ (GTCYCLE *this);
void                   _GTCYCLE_labels  (GTCYCLE *this, CONST_STRPTR * value);

USHORT                 _GTCYCLE_active_ (GTCYCLE *this);
void                   _GTCYCLE_active  (GTCYCLE *this, USHORT value);

/***********************************************************************************
 *
 * GTPALETTE
 *
 ***********************************************************************************/

typedef struct GTPALETTE_ GTPALETTE;

struct GTPALETTE_
{
    CGTGadget      gadget;
    BOOL            disabled;
    USHORT          depth;
    UBYTE           color;
    UBYTE           colorOffset;
    USHORT          indicatorWidth;
    USHORT          indicatorHeight;
    UBYTE *         colorTable;
    USHORT          numColors;
};

void _GTPALETTE_CONSTRUCTOR (GTPALETTE *this,
                            CONST_STRPTR label, USHORT numColors, 
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTPALETTE_disabled_ (GTPALETTE *this);
void                   _GTPALETTE_disabled  (GTPALETTE *this, BOOL value);

UBYTE                  _GTPALETTE_color_ (GTPALETTE *this);
void                   _GTPALETTE_color  (GTPALETTE *this, UBYTE value);

UBYTE                  _GTPALETTE_colorOffset_ (GTPALETTE *this);
void                   _GTPALETTE_colorOffset  (GTPALETTE *this, UBYTE value);

USHORT                 _GTPALETTE_indicatorWidth_ (GTPALETTE *this);
void                   _GTPALETTE_indicatorWidth  (GTPALETTE *this, USHORT value);

USHORT                 _GTPALETTE_indicatorHeight_ (GTPALETTE *this);
void                   _GTPALETTE_indicatorHeight  (GTPALETTE *this, USHORT value);

UBYTE *                _GTPALETTE_colorTable_ (GTPALETTE *this);
void                   _GTPALETTE_colorTable  (GTPALETTE *this, UBYTE * value);

USHORT                 _GTPALETTE_numColors_ (GTPALETTE *this);
void                   _GTPALETTE_numColors  (GTPALETTE *this, USHORT value);

/***********************************************************************************
 *
 * GTLISTVIEW
 *
 ***********************************************************************************/

typedef struct GTLISTVIEW_ GTLISTVIEW;

struct GTLISTVIEW_
{
    CGTGadget      gadget;
    BOOL            disabled;
    SHORT           makeVisible;
    CExecList      *labels;
    BOOL            readOnly;
    USHORT          scrollWidth;
    USHORT          selected;
    USHORT          spacing;
};

void _GTLISTVIEW_CONSTRUCTOR (GTLISTVIEW *this,
                              CONST_STRPTR label, struct List * labels,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTLISTVIEW_disabled_ (GTLISTVIEW *this);
void                   _GTLISTVIEW_disabled  (GTLISTVIEW *this, BOOL value);

SHORT                  _GTLISTVIEW_makeVisible_ (GTLISTVIEW *this);
void                   _GTLISTVIEW_makeVisible  (GTLISTVIEW *this, SHORT value);

struct List *          _GTLISTVIEW_labels_ (GTLISTVIEW *this);
void                   _GTLISTVIEW_labels  (GTLISTVIEW *this, struct List * value);

BOOL                   _GTLISTVIEW_readOnly_ (GTLISTVIEW *this);
void                   _GTLISTVIEW_readOnly  (GTLISTVIEW *this, BOOL value);

USHORT                 _GTLISTVIEW_scrollWidth_ (GTLISTVIEW *this);
void                   _GTLISTVIEW_scrollWidth  (GTLISTVIEW *this, USHORT value);

USHORT                 _GTLISTVIEW_selected_ (GTLISTVIEW *this);
void                   _GTLISTVIEW_selected  (GTLISTVIEW *this, USHORT value);

USHORT                 _GTLISTVIEW_spacing_ (GTLISTVIEW *this);
void                   _GTLISTVIEW_spacing  (GTLISTVIEW *this, USHORT value);


/********************************************
 *
 * common functions
 *
 ********************************************/

void        GTGADGETS_DEPLOY   (void);
void        GTGADGETS_FREE     (void);

void        GTG_DRAW_BEVEL_BOX (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, BOOL recessed );

SHORT       _GTGADGET_NEXT_ID (void);

/********************************************
 *
 * private data
 *
 ********************************************/

typedef struct
{
    CGTGadget        *first;
    CGTGadget        *last;
    struct Gadget     *gad;
    struct Gadget     *gadList;
    APTR              *vinfo;
    struct TextAttr    ta;
    BOOL               close_cb_installed;
    BOOL               msg_cb_installed;
    BOOL               deployed;
    SHORT              id;
} gt_win_ext_t;

extern gt_win_ext_t    _g_gt_win_ext[MAX_NUM_WINDOWS];

#endif

