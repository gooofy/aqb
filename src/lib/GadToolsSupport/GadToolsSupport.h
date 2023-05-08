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

void _CGTGADGET_CONSTRUCTOR (CGTGadget *this, CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// CGTGadget properties
SHORT        _CGTGADGET_X1_        (CGTGadget *this);
void         _CGTGADGET_X1         (CGTGadget *this, SHORT x1);
SHORT        _CGTGADGET_Y1_        (CGTGadget *this);
void         _CGTGADGET_Y1         (CGTGadget *this, SHORT y1);
SHORT        _CGTGADGET_X2_        (CGTGadget *this);
void         _CGTGADGET_X2         (CGTGadget *this, SHORT x2);
SHORT        _CGTGADGET_Y2_        (CGTGadget *this);
void         _CGTGADGET_Y2         (CGTGadget *this, SHORT y2);
CONST_STRPTR _CGTGADGET_TEXT_      (CGTGadget *this);
void         _CGTGADGET_TEXT       (CGTGadget *this, STRPTR text);
SHORT        _CGTGADGET_ID_        (CGTGadget *this);
void         _CGTGADGET_ID         (CGTGadget *this, SHORT id);
ULONG        _CGTGADGET_FLAGS_     (CGTGadget *this);
void         _CGTGADGET_FLAGS      (CGTGadget *this, ULONG flags);
BOOL         _CGTGADGET_DEPLOYED_  (CGTGadget *this);

/***********************************************************************************
 *
 * CGTButton
 *
 ***********************************************************************************/

typedef struct CGTButton_   CGTButton;
struct CGTButton_
{
    CGTGadget       gadget;
    BOOL            disabled;
    BOOL            immediate;
};

void _CGTBUTTON_CONSTRUCTOR (CGTButton *this, CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// CGTButton properties
BOOL _CGTBUTTON_DISABLED_  (CGTButton *this);
void _CGTBUTTON_DISABLED   (CGTButton *this, BOOL disabled);

BOOL _CGTBUTTON_IMMEDIATE_ (CGTButton *this);
void _CGTBUTTON_IMMEDIATE  (CGTButton *this, BOOL value);

/***********************************************************************************
 *
 * CGTCheckBox
 *
 ***********************************************************************************/

typedef struct CGTCheckBox_ CGTCheckBox;
struct CGTCheckBox_
{
    CGTGadget       gadget;
    BOOL            disabled;
    BOOL            checked;
    BOOL            scaled;
};

void _CGTCHECKBOX_CONSTRUCTOR (CGTCheckBox *this, CONST_STRPTR label,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

// CGTCheckBox properties
BOOL _CGTCHECKBOX_DISABLED_ (CGTCheckBox *this);
void _CGTCHECKBOX_DISABLED  (CGTCheckBox *this, BOOL disabled);
BOOL _CGTCHECKBOX_CHECKED_  (CGTCheckBox *this);
void _CGTCHECKBOX_CHECKED   (CGTCheckBox *this, BOOL checked);
BOOL _CGTCHECKBOX_SCALED_   (CGTCheckBox *this);
void _CGTCHECKBOX_SCALED    (CGTCheckBox *this, BOOL scaled);

/***********************************************************************************
 *
 * CGTSlider
 *
 ***********************************************************************************/

typedef struct CGTSlider_   CGTSlider;
struct CGTSlider_
{
    CGTGadget       gadget;
    BOOL            disabled;
    SHORT           min, max, level;
    ULONG           freedom;
    SHORT           maxLevelLen;
    CONST_STRPTR    levelFormat;
    ULONG           levelPlace;
    BOOL            immediate, relVerify;
};

void _CGTSLIDER_CONSTRUCTOR (CGTSlider *this, CONST_STRPTR label,
                            SHORT min, SHORT max, SHORT level, ULONG orient,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// CGTSlider properties
BOOL          _CGTSLIDER_DISABLED_    (CGTSlider *this);
void          _CGTSLIDER_DISABLED     (CGTSlider *this, BOOL disabled);

SHORT         _CGTSLIDER_MIN_         (CGTSlider *this);
void          _CGTSLIDER_MIN          (CGTSlider *this, SHORT i);

SHORT         _CGTSLIDER_MAX_         (CGTSlider *this);
void          _CGTSLIDER_MAX          (CGTSlider *this, SHORT i);

SHORT         _CGTSLIDER_LEVEL_       (CGTSlider *this);
void          _CGTSLIDER_LEVEL        (CGTSlider *this, SHORT i);

SHORT         _CGTSLIDER_MAXLEVELLEN_ (CGTSlider *this);
void          _CGTSLIDER_MAXLEVELLEN  (CGTSlider *this, SHORT i);

CONST_STRPTR  _CGTSLIDER_LEVELFORMAT_ (CGTSlider *this);
void          _CGTSLIDER_LEVELFORMAT  (CGTSlider *this, CONST_STRPTR s);

ULONG         _CGTSLIDER_LEVELPLACE_  (CGTSlider *this);
void          _CGTSLIDER_LEVELPLACE   (CGTSlider *this, ULONG u);

BOOL          _CGTSLIDER_IMMEDIATE_   (CGTSlider *this);
void          _CGTSLIDER_IMMEDIATE    (CGTSlider *this, BOOL b);

BOOL          _CGTSLIDER_RELVERIFY_   (CGTSlider *this);
void          _CGTSLIDER_RELVERIFY    (CGTSlider *this, BOOL b);

ULONG         _CGTSLIDER_FREEDOM_     (CGTSlider *this);
void          _CGTSLIDER_FREEDOM      (CGTSlider *this, ULONG u);

/***********************************************************************************
 *
 * CGTText
 *
 ***********************************************************************************/

typedef struct CGTText_ CGTText;

struct CGTText_
{
    CGTGadget       gadget;
    CONST_STRPTR    text;
    BOOL            copyText;
    BOOL            border;
    UBYTE           frontPen;
    UBYTE           backPen;
    UBYTE           justification;
    BOOL            clipped;
};

void _CGTTEXT_CONSTRUCTOR (CGTText *this, CONST_STRPTR label, CONST_STRPTR text,
                          BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                          void *user_data, ULONG flags, ULONG underscore);

CONST_STRPTR           _CGTTEXT_TEXT_          (CGTText *this);
void                   _CGTTEXT_TEXT           (CGTText *this, CONST_STRPTR value);

BOOL                   _CGTTEXT_COPYTEXT_      (CGTText *this);
void                   _CGTTEXT_COPYTEXT       (CGTText *this, BOOL value);

BOOL                   _CGTTEXT_BORDER_        (CGTText *this);
void                   _CGTTEXT_BORDER         (CGTText *this, BOOL value);

UBYTE                  _CGTTEXT_FRONTPEN_      (CGTText *this);
void                   _CGTTEXT_FRONTPEN       (CGTText *this, UBYTE value);

UBYTE                  _CGTTEXT_BACKPEN_       (CGTText *this);
void                   _CGTTEXT_BACKPEN        (CGTText *this, UBYTE value);

UBYTE                  _CGTTEXT_JUSTIFICATION_ (CGTText *this);
void                   _CGTTEXT_JUSTIFICATION  (CGTText *this, UBYTE value);

BOOL                   _CGTTEXT_CLIPPED_       (CGTText *this);
void                   _CGTTEXT_CLIPPED        (CGTText *this, BOOL value);

/***********************************************************************************
 *
 * CGTScroller
 *
 ***********************************************************************************/

typedef struct CGTScroller_ CGTScroller;

struct CGTScroller_
{
    CGTGadget       gadget;
    BOOL            disabled;
    BOOL            relVerify;
    BOOL            immediate;
    SHORT           top;
    SHORT           total;
    SHORT           visible;
    USHORT          arrows;
    ULONG           freedom;
};

void _CGTSCROLLER_CONSTRUCTOR (CGTScroller *this, CONST_STRPTR label,
                              SHORT top, SHORT total, SHORT visible, ULONG freedom,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTSCROLLER_DISABLED_  (CGTScroller *this);
void                   _CGTSCROLLER_DISABLED   (CGTScroller *this, BOOL value);

BOOL                   _CGTSCROLLER_RELVERIFY_ (CGTScroller *this);
void                   _CGTSCROLLER_RELVERIFY  (CGTScroller *this, BOOL value);

BOOL                   _CGTSCROLLER_IMMEDIATE_ (CGTScroller *this);
void                   _CGTSCROLLER_IMMEDIATE  (CGTScroller *this, BOOL value);

SHORT                  _CGTSCROLLER_TOP_       (CGTScroller *this);
void                   _CGTSCROLLER_TOP        (CGTScroller *this, SHORT value);

SHORT                  _CGTSCROLLER_TOTAL_     (CGTScroller *this);
void                   _CGTSCROLLER_TOTAL      (CGTScroller *this, SHORT value);

BOOL                   _CGTSCROLLER_VISIBLE_   (CGTScroller *this);
void                   _CGTSCROLLER_VISIBLE    (CGTScroller *this, BOOL value);

USHORT                 _CGTSCROLLER_ARROWS_    (CGTScroller *this);
void                   _CGTSCROLLER_ARROWS     (CGTScroller *this, USHORT value);

ULONG                  _CGTSCROLLER_FREEDOM_   (CGTScroller *this);
void                   _CGTSCROLLER_FREEDOM    (CGTScroller *this, ULONG value);

/***********************************************************************************
 *
 * CGTString
 *
 ***********************************************************************************/

typedef struct CGTString_ CGTString;

struct CGTString_
{
    CGTGadget       gadget;
    BOOL            disabled;
    BOOL            immediate;
    BOOL            tabCycle;
    CONST_STRPTR    str;
    USHORT          maxChars;
    BOOL            exitHelp;
    CONST_STRPTR    justification;
    BOOL            replaceMode;
};

void _CGTSTRING_CONSTRUCTOR (CGTString *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTSTRING_DISABLED_      (CGTString *this);
void                   _CGTSTRING_DISABLED       (CGTString *this, BOOL value);

BOOL                   _CGTSTRING_IMMEDIATE_     (CGTString *this);
void                   _CGTSTRING_IMMEDIATE      (CGTString *this, BOOL value);

BOOL                   _CGTSTRING_TABCYCLE_      (CGTString *this);
void                   _CGTSTRING_TABCYCLE       (CGTString *this, BOOL value);

CONST_STRPTR           _CGTSTRING_STR_           (CGTString *this);
void                   _CGTSTRING_STR            (CGTString *this, CONST_STRPTR value);

USHORT                 _CGTSTRING_MAXCHARS_      (CGTString *this);
void                   _CGTSTRING_MAXCHARS       (CGTString *this, USHORT value);

BOOL                   _CGTSTRING_EXITHELP_      (CGTString *this);
void                   _CGTSTRING_EXITHELP       (CGTString *this, BOOL value);

CONST_STRPTR           _CGTSTRING_JUSTIFICATION_ (CGTString *this);
void                   _CGTSTRING_JUSTIFICATION  (CGTString *this, CONST_STRPTR value);

BOOL                   _CGTSTRING_REPLACEMODE_   (CGTString *this);
void                   _CGTSTRING_REPLACEMODE    (CGTString *this, BOOL value);

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

void _CGTINTEGER_CONSTRUCTOR (CGTInteger *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTINTEGER_DISABLED_      (CGTInteger *this);
void                   _CGTINTEGER_DISABLED       (CGTInteger *this, BOOL value);

BOOL                   _CGTINTEGER_IMMEDIATE_     (CGTInteger *this);
void                   _CGTINTEGER_IMMEDIATE      (CGTInteger *this, BOOL value);

BOOL                   _CGTINTEGER_TABCYCLE_      (CGTInteger *this);
void                   _CGTINTEGER_TABCYCLE       (CGTInteger *this, BOOL value);

LONG                   _CGTINTEGER_NUMBER_        (CGTInteger *this);
void                   _CGTINTEGER_NUMBER         (CGTInteger *this, LONG value);

USHORT                 _CGTINTEGER_MAXCHARS_      (CGTInteger *this);
void                   _CGTINTEGER_MAXCHARS       (CGTInteger *this, USHORT value);

BOOL                   _CGTINTEGER_EXITHELP_      (CGTInteger *this);
void                   _CGTINTEGER_EXITHELP       (CGTInteger *this, BOOL value);

CONST_STRPTR           _CGTINTEGER_JUSTIFICATION_ (CGTInteger *this);
void                   _CGTINTEGER_JUSTIFICATION  (CGTInteger *this, CONST_STRPTR value);

BOOL                   _CGTINTEGER_REPLACEMODE_   (CGTInteger *this);
void                   _CGTINTEGER_REPLACEMODE    (CGTInteger *this, BOOL value);

/***********************************************************************************
 *
 * CGTNumber
 *
 ***********************************************************************************/

typedef struct CGTNumber_ CGTNumber;

struct CGTNumber_
{
    CGTGadget       gadget;
    LONG            number;
    BOOL            border;
    UBYTE           frontPen;
    UBYTE           backPen;
    UBYTE           justification;
    CONST_STRPTR    format;
    ULONG           maxNumberLen;
    BOOL            clipped;
};

void _CGTNUMBER_CONSTRUCTOR (CGTNumber *this,
                            CONST_STRPTR label, LONG number,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

LONG                   _CGTNUMBER_NUMBER_        (CGTNumber *this);
void                   _CGTNUMBER_NUMBER         (CGTNumber *this, LONG value);

BOOL                   _CGTNUMBER_BORDER_        (CGTNumber *this);
void                   _CGTNUMBER_BORDER         (CGTNumber *this, BOOL value);

UBYTE                  _CGTNUMBER_FRONTPEN_      (CGTNumber *this);
void                   _CGTNUMBER_FRONTPEN       (CGTNumber *this, UBYTE value);

UBYTE                  _CGTNUMBER_BACKPEN_       (CGTNumber *this);
void                   _CGTNUMBER_BACKPEN        (CGTNumber *this, UBYTE value);

UBYTE                  _CGTNUMBER_JUSTIFICATION_ (CGTNumber *this);
void                   _CGTNUMBER_JUSTIFICATION  (CGTNumber *this, UBYTE value);

CONST_STRPTR           _CGTNUMBER_FORMAT_        (CGTNumber *this);
void                   _CGTNUMBER_FORMAT         (CGTNumber *this, CONST_STRPTR value);

ULONG                  _CGTNUMBER_MAXNUMBERLEN_  (CGTNumber *this);
void                   _CGTNUMBER_MAXNUMBERLEN   (CGTNumber *this, ULONG value);

BOOL                   _CGTNUMBER_CLIPPED_       (CGTNumber *this);
void                   _CGTNUMBER_CLIPPED        (CGTNumber *this, BOOL value);

/***********************************************************************************
 *
 * CGTMX
 *
 ***********************************************************************************/

typedef struct CGTMX_ CGTMX;

struct CGTMX_
{
    CGTGadget       gadget;
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

BOOL                   _CGTMX_DISABLED_   (CGTMX *this);
void                   _CGTMX_DISABLED    (CGTMX *this, BOOL value);

CONST_STRPTR *         _CGTMX_LABELS_     (CGTMX *this);
void                   _CGTMX_LABELS      (CGTMX *this, CONST_STRPTR * value);

USHORT                 _CGTMX_ACTIVE_     (CGTMX *this);
void                   _CGTMX_ACTIVE      (CGTMX *this, USHORT value);

USHORT                 _CGTMX_SPACING_    (CGTMX *this);
void                   _CGTMX_SPACING     (CGTMX *this, USHORT value);

BOOL                   _CGTMX_SCALED_     (CGTMX *this);
void                   _CGTMX_SCALED      (CGTMX *this, BOOL value);

ULONG                  _CGTMX_TITLEPLACE_ (CGTMX *this);
void                   _CGTMX_TITLEPLACE  (CGTMX *this, ULONG value);

/***********************************************************************************
 *
 * CGTCycle
 *
 ***********************************************************************************/

typedef struct CGTCycle_ CGTCycle;

struct CGTCycle_
{
    CGTGadget       gadget;
    BOOL            disabled;
    CONST_STRPTR *  labels;
    USHORT          active;
};

void _CGTCYCLE_CONSTRUCTOR (CGTCycle *this,
                            CONST_STRPTR label, CONST_STRPTR * labels,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTCYCLE_DISABLED_ (CGTCycle *this);
void                   _CGTCYCLE_DISABLED  (CGTCycle *this, BOOL value);

CONST_STRPTR *         _CGTCYCLE_LABELS_   (CGTCycle *this);
void                   _CGTCYCLE_LABELS    (CGTCycle *this, CONST_STRPTR * value);

USHORT                 _CGTCYCLE_ACTIVE_   (CGTCycle *this);
void                   _CGTCYCLE_ACTIVE    (CGTCycle *this, USHORT value);

/***********************************************************************************
 *
 * CGTPalette
 *
 ***********************************************************************************/

typedef struct CGTPalette_ CGTPalette;

struct CGTPalette_
{
    CGTGadget       gadget;
    BOOL            disabled;
    UBYTE           color;
    UBYTE           colorOffset;
    USHORT          indicatorWidth;
    USHORT          indicatorHeight;
    UBYTE *         colorTable;
    USHORT          numColors;
};

void _CGTPALETTE_CONSTRUCTOR (CGTPalette *this,
                              CONST_STRPTR label, USHORT numColors,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTPALETTE_DISABLED_         (CGTPalette *this);
void                   _CGTPALETTE_DISABLED          (CGTPalette *this, BOOL value);

UBYTE                  _CGTPALETTE_COLOR_            (CGTPalette *this);
void                   _CGTPALETTE_COLOR             (CGTPalette *this, UBYTE value);

UBYTE                  _CGTPALETTE_COLOROFFSET_      (CGTPalette *this);
void                   _CGTPALETTE_COLOROFFSET       (CGTPalette *this, UBYTE value);

USHORT                 _CGTPALETTE_INDICATORWIDTH_   (CGTPalette *this);
void                   _CGTPALETTE_INDICATORWIDTH    (CGTPalette *this, USHORT value);

USHORT                 _CGTPALETTE_INDICATORHEIGHT_  (CGTPalette *this);
void                   _CGTPALETTE_INDICATORHEIGHT   (CGTPalette *this, USHORT value);

UBYTE *                _CGTPALETTE_COLORTABLE_       (CGTPalette *this);
void                   _CGTPALETTE_COLORTABLE        (CGTPalette *this, UBYTE * value);

USHORT                 _CGTPALETTE_NUMCOLORS_        (CGTPalette *this);
void                   _CGTPALETTE_NUMCOLORS         (CGTPalette *this, USHORT value);

/***********************************************************************************
 *
 * CGTListView
 *
 ***********************************************************************************/

typedef struct CGTListView_ CGTListView;

struct CGTListView_
{
    CGTGadget       gadget;
    BOOL            disabled;
    SHORT           makeVisible;
    CExecList      *labels;
    BOOL            readOnly;
    USHORT          scrollWidth;
    USHORT          selected;
    USHORT          spacing;
};

void _CGTLISTVIEW_CONSTRUCTOR (CGTListView *this,
                               CONST_STRPTR label, CExecList *labels,
                               BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                               void *user_data, ULONG flags, ULONG underscore);

BOOL                   _CGTLISTVIEW_DISABLED_    (CGTListView *this);
void                   _CGTLISTVIEW_DISABLED     (CGTListView *this, BOOL value);

SHORT                  _CGTLISTVIEW_MAKEVISIBLE_ (CGTListView *this);
void                   _CGTLISTVIEW_MAKEVISIBLE  (CGTListView *this, SHORT value);

CExecList             *_CGTLISTVIEW_LABELS_      (CGTListView *this);
void                   _CGTLISTVIEW_LABELS       (CGTListView *this, CExecList * value);

BOOL                   _CGTLISTVIEW_READONLY_    (CGTListView *this);
void                   _CGTLISTVIEW_READONLY     (CGTListView *this, BOOL value);

USHORT                 _CGTLISTVIEW_SCROLLWIDTH_ (CGTListView *this);
void                   _CGTLISTVIEW_SCROLLWIDTH  (CGTListView *this, USHORT value);

USHORT                 _CGTLISTVIEW_SELECTED_    (CGTListView *this);
void                   _CGTLISTVIEW_SELECTED     (CGTListView *this, USHORT value);

USHORT                 _CGTLISTVIEW_SPACING_     (CGTListView *this);
void                   _CGTLISTVIEW_SPACING      (CGTListView *this, USHORT value);


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

gt_win_ext_t *_gt_get_ext (SHORT win_id);

#endif

