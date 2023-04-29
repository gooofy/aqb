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
 * CGTSlider
 *
 ***********************************************************************************/

typedef struct CGTSlider_   CGTSlider;
struct CGTSlider_
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

void _CGTSlider_CONSTRUCTOR (CGTSlider *this, CONST_STRPTR label,
                            SHORT min, SHORT max, SHORT level, ULONG orient,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// CGTSlider properties
BOOL          _CGTSlider_disabled_ (CGTSlider *this);
void          _CGTSlider_disabled  (CGTSlider *this, BOOL disabled);

SHORT         _CGTSlider_min_ (CGTSlider *this);
void          _CGTSlider_min  (CGTSlider *this, SHORT i);

SHORT         _CGTSlider_max_ (CGTSlider *this);
void          _CGTSlider_max  (CGTSlider *this, SHORT i);

SHORT         _CGTSlider_level_ (CGTSlider *this);
void          _CGTSlider_level  (CGTSlider *this, SHORT i);

SHORT         _CGTSlider_maxLevelLen_ (CGTSlider *this);
void          _CGTSlider_maxLevelLen  (CGTSlider *this, SHORT i);

CONST_STRPTR  _CGTSlider_levelFormat_ (CGTSlider *this);
void          _CGTSlider_levelFormat  (CGTSlider *this, CONST_STRPTR s);

ULONG         _CGTSlider_levelPlace_ (CGTSlider *this);
void          _CGTSlider_levelPlace  (CGTSlider *this, ULONG u);

BOOL          _CGTSlider_immediate_ (CGTSlider *this);
void          _CGTSlider_immediate  (CGTSlider *this, BOOL b);

BOOL          _CGTSlider_relVerify_ (CGTSlider *this);
void          _CGTSlider_relVerify  (CGTSlider *this, BOOL b);

ULONG         _CGTSlider_freedom_ (CGTSlider *this);
void          _CGTSlider_freedom  (CGTSlider *this, ULONG u);

/***********************************************************************************
 *
 * CGTText
 *
 ***********************************************************************************/

typedef struct CGTText_ CGTText;

struct CGTText_
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

void _CGTText_CONSTRUCTOR (CGTText *this, CONST_STRPTR label, CONST_STRPTR text,
                          BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                          void *user_data, ULONG flags, ULONG underscore);

CONST_STRPTR           _CGTText_text_ (CGTText *this);
void                   _CGTText_text  (CGTText *this, CONST_STRPTR value);

BOOL                   _CGTText_copyText_ (CGTText *this);
void                   _CGTText_copyText  (CGTText *this, BOOL value);

BOOL                   _CGTText_Border_ (CGTText *this);
void                   _CGTText_Border  (CGTText *this, BOOL value);

UBYTE                  _CGTText_frontPen_ (CGTText *this);
void                   _CGTText_frontPen  (CGTText *this, UBYTE value);

UBYTE                  _CGTText_backPen_ (CGTText *this);
void                   _CGTText_backPen  (CGTText *this, UBYTE value);

UBYTE                  _CGTText_justification_ (CGTText *this);
void                   _CGTText_justification  (CGTText *this, UBYTE value);

BOOL                   _CGTText_clipped_ (CGTText *this);
void                   _CGTText_clipped  (CGTText *this, BOOL value);

/***********************************************************************************
 *
 * CGTScroller
 *
 ***********************************************************************************/

typedef struct CGTScroller_ CGTScroller;

struct CGTScroller_
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

void _CGTScroller_CONSTRUCTOR (CGTScroller *this, CONST_STRPTR label,
                              SHORT top, SHORT total, SHORT visible, ULONG freedom,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTScroller_disabled_ (CGTScroller *this);
void                   _CGTScroller_disabled  (CGTScroller *this, BOOL value);

BOOL                   _CGTScroller_relVerify_ (CGTScroller *this);
void                   _CGTScroller_relVerify  (CGTScroller *this, BOOL value);

BOOL                   _CGTScroller_immediate_ (CGTScroller *this);
void                   _CGTScroller_immediate  (CGTScroller *this, BOOL value);

SHORT                  _CGTScroller_top_ (CGTScroller *this);
void                   _CGTScroller_top  (CGTScroller *this, SHORT value);

SHORT                  _CGTScroller_total_ (CGTScroller *this);
void                   _CGTScroller_total  (CGTScroller *this, SHORT value);

BOOL                   _CGTScroller_visible_ (CGTScroller *this);
void                   _CGTScroller_visible  (CGTScroller *this, BOOL value);

USHORT                 _CGTScroller_arrows_ (CGTScroller *this);
void                   _CGTScroller_arrows  (CGTScroller *this, USHORT value);

ULONG                  _CGTScroller_freedom_ (CGTScroller *this);
void                   _CGTScroller_freedom  (CGTScroller *this, ULONG value);

/***********************************************************************************
 *
 * CGTString
 *
 ***********************************************************************************/

typedef struct CGTString_ CGTString;

struct CGTString_
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

void _CGTString_CONSTRUCTOR (CGTString *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTString_disabled_ (CGTString *this);
void                   _CGTString_disabled  (CGTString *this, BOOL value);

BOOL                   _CGTString_immediate_ (CGTString *this);
void                   _CGTString_immediate  (CGTString *this, BOOL value);

BOOL                   _CGTString_tabCycle_ (CGTString *this);
void                   _CGTString_tabCycle  (CGTString *this, BOOL value);

CONST_STRPTR           _CGTString_str_ (CGTString *this);
void                   _CGTString_str  (CGTString *this, CONST_STRPTR value);

USHORT                 _CGTString_MaxChars_ (CGTString *this);
void                   _CGTString_MaxChars  (CGTString *this, USHORT value);

BOOL                   _CGTString_exitHelp_ (CGTString *this);
void                   _CGTString_exitHelp  (CGTString *this, BOOL value);

CONST_STRPTR           _CGTString_justification_ (CGTString *this);
void                   _CGTString_justification  (CGTString *this, CONST_STRPTR value);

BOOL                   _CGTString_replaceMode_ (CGTString *this);
void                   _CGTString_replaceMode  (CGTString *this, BOOL value);

/***********************************************************************************
 *
 * CGTInteger
 *
 ***********************************************************************************/

typedef struct CGTInteger_ CGTInteger;

struct CGTInteger_
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

void _CGTInteger_CONSTRUCTOR (CGTInteger *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTInteger_disabled_ (CGTInteger *this);
void                   _CGTInteger_disabled  (CGTInteger *this, BOOL value);

BOOL                   _CGTInteger_immediate_ (CGTInteger *this);
void                   _CGTInteger_immediate  (CGTInteger *this, BOOL value);

BOOL                   _CGTInteger_tabCycle_ (CGTInteger *this);
void                   _CGTInteger_tabCycle  (CGTInteger *this, BOOL value);

LONG                   _CGTInteger_number_ (CGTInteger *this);
void                   _CGTInteger_number  (CGTInteger *this, LONG value);

USHORT                 _CGTInteger_maxChars_ (CGTInteger *this);
void                   _CGTInteger_maxChars  (CGTInteger *this, USHORT value);

BOOL                   _CGTInteger_exitHelp_ (CGTInteger *this);
void                   _CGTInteger_exitHelp  (CGTInteger *this, BOOL value);

CONST_STRPTR           _CGTInteger_justification_ (CGTInteger *this);
void                   _CGTInteger_justification  (CGTInteger *this, CONST_STRPTR value);

BOOL                   _CGTInteger_replaceMode_ (CGTInteger *this);
void                   _CGTInteger_replaceMode  (CGTInteger *this, BOOL value);

/***********************************************************************************
 *
 * CGTNumber
 *
 ***********************************************************************************/

typedef struct CGTNumber_ CGTNumber;

struct CGTNumber_
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

void _CGTNumber_CONSTRUCTOR (CGTNumber *this,
                            CONST_STRPTR label, LONG number,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

LONG                   _CGTNumber_number_ (CGTNumber *this);
void                   _CGTNumber_number  (CGTNumber *this, LONG value);

BOOL                   _CGTNumber_Border_ (CGTNumber *this);
void                   _CGTNumber_Border  (CGTNumber *this, BOOL value);

UBYTE                  _CGTNumber_frontPen_ (CGTNumber *this);
void                   _CGTNumber_frontPen  (CGTNumber *this, UBYTE value);

UBYTE                  _CGTNumber_backPen_ (CGTNumber *this);
void                   _CGTNumber_backPen  (CGTNumber *this, UBYTE value);

UBYTE                  _CGTNumber_justification_ (CGTNumber *this);
void                   _CGTNumber_justification  (CGTNumber *this, UBYTE value);

CONST_STRPTR           _CGTNumber_format_ (CGTNumber *this);
void                   _CGTNumber_format  (CGTNumber *this, CONST_STRPTR value);

ULONG                  _CGTNumber_maxNumberLen_ (CGTNumber *this);
void                   _CGTNumber_maxNumberLen  (CGTNumber *this, ULONG value);

BOOL                   _CGTNumber_clipped_ (CGTNumber *this);
void                   _CGTNumber_clipped  (CGTNumber *this, BOOL value);

/***********************************************************************************
 *
 * CGTMX
 *
 ***********************************************************************************/

typedef struct CGTMX_ CGTMX;

struct CGTMX_
{
    CGTGadget      gadget;
    BOOL            disabled;
    CONST_STRPTR *  labels;
    USHORT          active;
    USHORT          spacing;
    BOOL            scaled;
    ULONG           titlePlace;
};

void _CGTMX_CONSTRUCTOR (CGTMX *this,
                            CONST_STRPTR label, CONST_STRPTR * labels, 
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTMX_disabled_ (CGTMX *this);
void                   _CGTMX_disabled  (CGTMX *this, BOOL value);

CONST_STRPTR *         _CGTMX_labels_ (CGTMX *this);
void                   _CGTMX_labels  (CGTMX *this, CONST_STRPTR * value);

USHORT                 _CGTMX_active_ (CGTMX *this);
void                   _CGTMX_active  (CGTMX *this, USHORT value);

USHORT                 _CGTMX_spacing_ (CGTMX *this);
void                   _CGTMX_spacing  (CGTMX *this, USHORT value);

BOOL                   _CGTMX_scaled_ (CGTMX *this);
void                   _CGTMX_scaled  (CGTMX *this, BOOL value);

ULONG                  _CGTMX_titlePlace_ (CGTMX *this);
void                   _CGTMX_titlePlace  (CGTMX *this, ULONG value);

/***********************************************************************************
 *
 * CGTCycle
 *
 ***********************************************************************************/

typedef struct CGTCycle_ CGTCycle;

struct CGTCycle_
{
    CGTGadget      gadget;
    BOOL            disabled;
    CONST_STRPTR *  labels;
    USHORT          active;
};

void _CGTCycle_CONSTRUCTOR (CGTCycle *this,
                            CONST_STRPTR label, CONST_STRPTR * labels,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTCycle_disabled_ (CGTCycle *this);
void                   _CGTCycle_disabled  (CGTCycle *this, BOOL value);

CONST_STRPTR *         _CGTCycle_labels_ (CGTCycle *this);
void                   _CGTCycle_labels  (CGTCycle *this, CONST_STRPTR * value);

USHORT                 _CGTCycle_active_ (CGTCycle *this);
void                   _CGTCycle_active  (CGTCycle *this, USHORT value);

/***********************************************************************************
 *
 * CGTPalette
 *
 ***********************************************************************************/

typedef struct CGTPalette_ CGTPalette;

struct CGTPalette_
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

void _CGTPalette_CONSTRUCTOR (CGTPalette *this,
                            CONST_STRPTR label, USHORT numColors, 
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTPalette_disabled_ (CGTPalette *this);
void                   _CGTPalette_disabled  (CGTPalette *this, BOOL value);

UBYTE                  _CGTPalette_COLOR_ (CGTPalette *this);
void                   _CGTPalette_COLOR  (CGTPalette *this, UBYTE value);

UBYTE                  _CGTPalette_colorOffset_ (CGTPalette *this);
void                   _CGTPalette_colorOffset  (CGTPalette *this, UBYTE value);

USHORT                 _CGTPalette_indicatorWidth_ (CGTPalette *this);
void                   _CGTPalette_indicatorWidth  (CGTPalette *this, USHORT value);

USHORT                 _CGTPalette_indicatorHeight_ (CGTPalette *this);
void                   _CGTPalette_indicatorHeight  (CGTPalette *this, USHORT value);

UBYTE *                _CGTPalette_colorTable_ (CGTPalette *this);
void                   _CGTPalette_colorTable  (CGTPalette *this, UBYTE * value);

USHORT                 _CGTPalette_numColors_ (CGTPalette *this);
void                   _CGTPalette_numColors  (CGTPalette *this, USHORT value);

/***********************************************************************************
 *
 * CGTListView
 *
 ***********************************************************************************/

typedef struct CGTListView_ CGTListView;

struct CGTListView_
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

void _CGTListView_CONSTRUCTOR (CGTListView *this,
                              CONST_STRPTR label, CExecList *labels,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTListView_disabled_ (CGTListView *this);
void                   _CGTListView_disabled  (CGTListView *this, BOOL value);

SHORT                  _CGTListView_makeVisible_ (CGTListView *this);
void                   _CGTListView_makeVisible  (CGTListView *this, SHORT value);

CExecList             *_CGTListView_labels_ (CGTListView *this);
void                   _CGTListView_labels  (CGTListView *this, CExecList * value);

BOOL                   _CGTListView_readOnly_ (CGTListView *this);
void                   _CGTListView_readOnly  (CGTListView *this, BOOL value);

USHORT                 _CGTListView_scrollWidth_ (CGTListView *this);
void                   _CGTListView_scrollWidth  (CGTListView *this, USHORT value);

USHORT                 _CGTListView_selected_ (CGTListView *this);
void                   _CGTListView_selected  (CGTListView *this, USHORT value);

USHORT                 _CGTListView_spacing_ (CGTListView *this);
void                   _CGTListView_spacing  (CGTListView *this, USHORT value);


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
    CGTGadget         *first;
    CGTGadget         *last;
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

