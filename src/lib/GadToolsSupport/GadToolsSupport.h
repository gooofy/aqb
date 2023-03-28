#ifndef HAVE_GADTOOLS_SUPPORT_H
#define HAVE_GADTOOLS_SUPPORT_H

#include <exec/types.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#include <libraries/gadtools.h>

#define AE_GTG_CREATE   400
#define AE_GTG_MODIFY   401
#define AE_GTG_DEPLOY   402
#define AE_GTG_SELECTED 403
#define AE_GTG_CALLBACK 404
#define AE_GTG_BUFFER   405
#define AE_GTG_NUM      406

/***********************************************************************************
 *
 * GTGADGET
 *
 ***********************************************************************************/

typedef struct GTGADGET_   GTGADGET_t;

typedef void (*gtgadget_cb_t)(GTGADGET_t *gtg, USHORT code);
typedef struct Gadget * (*gtgadget_deploy_cb_t)(GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta);

struct GTGADGET_
{
    gtgadget_cb_t         gadgetup_cb;
    gtgadget_cb_t         gadgetdown_cb;
    gtgadget_cb_t         gadgetmove_cb;

    void                 *user_data;
    ULONG                 underscore;

    GTGADGET_t           *prev, *next;

    struct NewGadget      ng;

    struct Gadget        *gad;
    struct Window        *win;

    gtgadget_deploy_cb_t  deploy_cb;
};

void _GTGADGET_CONSTRUCTOR (GTGADGET_t *this, CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// GTGADGET properties
SHORT        _GTGADGET_x1_        (GTGADGET_t *this);
void         _GTGADGET_x1         (GTGADGET_t *this, SHORT x1);
SHORT        _GTGADGET_y1_        (GTGADGET_t *this);
void         _GTGADGET_y1         (GTGADGET_t *this, SHORT y1);
SHORT        _GTGADGET_x2_        (GTGADGET_t *this);
void         _GTGADGET_x2         (GTGADGET_t *this, SHORT x2);
SHORT        _GTGADGET_y2_        (GTGADGET_t *this);
void         _GTGADGET_y2         (GTGADGET_t *this, SHORT y2);
CONST_STRPTR _GTGADGET_text_      (GTGADGET_t *this);
void         _GTGADGET_text       (GTGADGET_t *this, STRPTR text);
SHORT        _GTGADGET_id_        (GTGADGET_t *this);
void         _GTGADGET_id         (GTGADGET_t *this, SHORT id);
ULONG        _GTGADGET_flags_     (GTGADGET_t *this);
void         _GTGADGET_flags      (GTGADGET_t *this, ULONG flags);
BOOL         _GTGADGET_deployed_  (GTGADGET_t *this);

/***********************************************************************************
 *
 * GTBUTTON
 *
 ***********************************************************************************/

typedef struct GTBUTTON_   GTBUTTON_t;
struct GTBUTTON_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
    BOOL            immediate;
};

void _GTBUTTON_CONSTRUCTOR (GTBUTTON_t *this, CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// GTBUTTON properties
BOOL _GTBUTTON_disabled_ (GTBUTTON_t *this);
void _GTBUTTON_disabled  (GTBUTTON_t *this, BOOL disabled);

BOOL _GTBUTTON_immediate_ (GTBUTTON_t *this);
void _GTBUTTON_immediate  (GTBUTTON_t *this, BOOL value);

/***********************************************************************************
 *
 * GTCHECKBOX
 *
 ***********************************************************************************/

typedef struct GTCHECKBOX_ GTCHECKBOX_t;
struct GTCHECKBOX_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
    BOOL            checked;
    BOOL            scaled;
};

void _GTCHECKBOX_CONSTRUCTOR (GTCHECKBOX_t *this, CONST_STRPTR label,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

// GTCHECKBOX properties
BOOL _GTCHECKBOX_disabled_ (GTCHECKBOX_t *this);
void _GTCHECKBOX_disabled  (GTCHECKBOX_t *this, BOOL disabled);
BOOL _GTCHECKBOX_checked_  (GTCHECKBOX_t *this);
void _GTCHECKBOX_checked   (GTCHECKBOX_t *this, BOOL checked);
BOOL _GTCHECKBOX_scaled_   (GTCHECKBOX_t *this);
void _GTCHECKBOX_scaled    (GTCHECKBOX_t *this, BOOL scaled);

/***********************************************************************************
 *
 * GTSLIDER
 *
 ***********************************************************************************/

typedef struct GTSLIDER_   GTSLIDER_t;
struct GTSLIDER_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
    SHORT           min, max, level;
    ULONG           freedom;
    SHORT           maxLevelLen;
    CONST_STRPTR    levelFormat;
    ULONG           levelPlace;
    BOOL            immediate, relVerify;
};

void _GTSLIDER_CONSTRUCTOR (GTSLIDER_t *this, CONST_STRPTR label,
                            SHORT min, SHORT max, SHORT level, ULONG orient,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// GTSLIDER properties
BOOL          _GTSLIDER_disabled_ (GTSLIDER_t *this);
void          _GTSLIDER_disabled  (GTSLIDER_t *this, BOOL disabled);

SHORT         _GTSLIDER_min_ (GTSLIDER_t *this);
void          _GTSLIDER_min  (GTSLIDER_t *this, SHORT i);

SHORT         _GTSLIDER_max_ (GTSLIDER_t *this);
void          _GTSLIDER_max  (GTSLIDER_t *this, SHORT i);

SHORT         _GTSLIDER_level_ (GTSLIDER_t *this);
void          _GTSLIDER_level  (GTSLIDER_t *this, SHORT i);

SHORT         _GTSLIDER_maxLevelLen_ (GTSLIDER_t *this);
void          _GTSLIDER_maxLevelLen  (GTSLIDER_t *this, SHORT i);

CONST_STRPTR  _GTSLIDER_levelFormat_ (GTSLIDER_t *this);
void          _GTSLIDER_levelFormat  (GTSLIDER_t *this, CONST_STRPTR s);

ULONG         _GTSLIDER_levelPlace_ (GTSLIDER_t *this);
void          _GTSLIDER_levelPlace  (GTSLIDER_t *this, ULONG u);

BOOL          _GTSLIDER_immediate_ (GTSLIDER_t *this);
void          _GTSLIDER_immediate  (GTSLIDER_t *this, BOOL b);

BOOL          _GTSLIDER_relVerify_ (GTSLIDER_t *this);
void          _GTSLIDER_relVerify  (GTSLIDER_t *this, BOOL b);

ULONG         _GTSLIDER_freedom_ (GTSLIDER_t *this);
void          _GTSLIDER_freedom  (GTSLIDER_t *this, ULONG u);

/***********************************************************************************
 *
 * GTTEXT
 *
 ***********************************************************************************/

typedef struct GTTEXT_ GTTEXT_t;

struct GTTEXT_
{
    GTGADGET_t      gadget;
    CONST_STRPTR    text;
    BOOL            copyText;
    BOOL            border;
    UBYTE           frontPen;
    UBYTE           backPen;
    UBYTE           justification;
    BOOL            clipped;
};

void _GTTEXT_CONSTRUCTOR (GTTEXT_t *this, CONST_STRPTR label, CONST_STRPTR text,
                          BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                          void *user_data, ULONG flags, ULONG underscore);

CONST_STRPTR           _GTTEXT_text_ (GTTEXT_t *this);
void                   _GTTEXT_text  (GTTEXT_t *this, CONST_STRPTR value);

BOOL                   _GTTEXT_copyText_ (GTTEXT_t *this);
void                   _GTTEXT_copyText  (GTTEXT_t *this, BOOL value);

BOOL                   _GTTEXT_border_ (GTTEXT_t *this);
void                   _GTTEXT_border  (GTTEXT_t *this, BOOL value);

UBYTE                  _GTTEXT_frontPen_ (GTTEXT_t *this);
void                   _GTTEXT_frontPen  (GTTEXT_t *this, UBYTE value);

UBYTE                  _GTTEXT_backPen_ (GTTEXT_t *this);
void                   _GTTEXT_backPen  (GTTEXT_t *this, UBYTE value);

UBYTE                  _GTTEXT_justification_ (GTTEXT_t *this);
void                   _GTTEXT_justification  (GTTEXT_t *this, UBYTE value);

BOOL                   _GTTEXT_clipped_ (GTTEXT_t *this);
void                   _GTTEXT_clipped  (GTTEXT_t *this, BOOL value);

/***********************************************************************************
 *
 * GTSCROLLER
 *
 ***********************************************************************************/

typedef struct GTSCROLLER_ GTSCROLLER_t;

struct GTSCROLLER_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
    BOOL            relVerify;
    BOOL            immediate;
    SHORT           top;
    SHORT           total;
    SHORT           visible;
    USHORT          arrows;
    ULONG           freedom;
};

void _GTSCROLLER_CONSTRUCTOR (GTSCROLLER_t *this, CONST_STRPTR label,
                              SHORT top, SHORT total, SHORT visible, ULONG freedom,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTSCROLLER_disabled_ (GTSCROLLER_t *this);
void                   _GTSCROLLER_disabled  (GTSCROLLER_t *this, BOOL value);

BOOL                   _GTSCROLLER_relVerify_ (GTSCROLLER_t *this);
void                   _GTSCROLLER_relVerify  (GTSCROLLER_t *this, BOOL value);

BOOL                   _GTSCROLLER_immediate_ (GTSCROLLER_t *this);
void                   _GTSCROLLER_immediate  (GTSCROLLER_t *this, BOOL value);

SHORT                  _GTSCROLLER_top_ (GTSCROLLER_t *this);
void                   _GTSCROLLER_top  (GTSCROLLER_t *this, SHORT value);

SHORT                  _GTSCROLLER_total_ (GTSCROLLER_t *this);
void                   _GTSCROLLER_total  (GTSCROLLER_t *this, SHORT value);

BOOL                   _GTSCROLLER_visible_ (GTSCROLLER_t *this);
void                   _GTSCROLLER_visible  (GTSCROLLER_t *this, BOOL value);

USHORT                 _GTSCROLLER_arrows_ (GTSCROLLER_t *this);
void                   _GTSCROLLER_arrows  (GTSCROLLER_t *this, USHORT value);

ULONG                  _GTSCROLLER_freedom_ (GTSCROLLER_t *this);
void                   _GTSCROLLER_freedom  (GTSCROLLER_t *this, ULONG value);

/***********************************************************************************
 *
 * GTSTRING
 *
 ***********************************************************************************/

typedef struct GTSTRING_ GTSTRING_t;

struct GTSTRING_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
    BOOL            immediate;
    BOOL            tabCycle;
    CONST_STRPTR    str;
    USHORT          maxChars;
    BOOL            exitHelp;
    CONST_STRPTR    justification;
    BOOL            replaceMode;
};

void _GTSTRING_CONSTRUCTOR (GTSTRING_t *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTSTRING_disabled_ (GTSTRING_t *this);
void                   _GTSTRING_disabled  (GTSTRING_t *this, BOOL value);

BOOL                   _GTSTRING_immediate_ (GTSTRING_t *this);
void                   _GTSTRING_immediate  (GTSTRING_t *this, BOOL value);

BOOL                   _GTSTRING_tabCycle_ (GTSTRING_t *this);
void                   _GTSTRING_tabCycle  (GTSTRING_t *this, BOOL value);

CONST_STRPTR           _GTSTRING_str_ (GTSTRING_t *this);
void                   _GTSTRING_str  (GTSTRING_t *this, CONST_STRPTR value);

USHORT                 _GTSTRING_maxChars_ (GTSTRING_t *this);
void                   _GTSTRING_maxChars  (GTSTRING_t *this, USHORT value);

BOOL                   _GTSTRING_exitHelp_ (GTSTRING_t *this);
void                   _GTSTRING_exitHelp  (GTSTRING_t *this, BOOL value);

CONST_STRPTR           _GTSTRING_justification_ (GTSTRING_t *this);
void                   _GTSTRING_justification  (GTSTRING_t *this, CONST_STRPTR value);

BOOL                   _GTSTRING_replaceMode_ (GTSTRING_t *this);
void                   _GTSTRING_replaceMode  (GTSTRING_t *this, BOOL value);

/***********************************************************************************
 *
 * GTINTEGER
 *
 ***********************************************************************************/

typedef struct GTINTEGER_ GTINTEGER_t;

struct GTINTEGER_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
    BOOL            immediate;
    BOOL            tabCycle;
    LONG            number;
    USHORT          maxChars;
    BOOL            exitHelp;
    CONST_STRPTR    justification;
    BOOL            replaceMode;
};

void _GTINTEGER_CONSTRUCTOR (GTINTEGER_t *this,
                            CONST_STRPTR label,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

BOOL                   _GTINTEGER_disabled_ (GTINTEGER_t *this);
void                   _GTINTEGER_disabled  (GTINTEGER_t *this, BOOL value);

BOOL                   _GTINTEGER_immediate_ (GTINTEGER_t *this);
void                   _GTINTEGER_immediate  (GTINTEGER_t *this, BOOL value);

BOOL                   _GTINTEGER_tabCycle_ (GTINTEGER_t *this);
void                   _GTINTEGER_tabCycle  (GTINTEGER_t *this, BOOL value);

LONG                   _GTINTEGER_number_ (GTINTEGER_t *this);
void                   _GTINTEGER_number  (GTINTEGER_t *this, LONG value);

USHORT                 _GTINTEGER_maxChars_ (GTINTEGER_t *this);
void                   _GTINTEGER_maxChars  (GTINTEGER_t *this, USHORT value);

BOOL                   _GTINTEGER_exitHelp_ (GTINTEGER_t *this);
void                   _GTINTEGER_exitHelp  (GTINTEGER_t *this, BOOL value);

CONST_STRPTR           _GTINTEGER_justification_ (GTINTEGER_t *this);
void                   _GTINTEGER_justification  (GTINTEGER_t *this, CONST_STRPTR value);

BOOL                   _GTINTEGER_replaceMode_ (GTINTEGER_t *this);
void                   _GTINTEGER_replaceMode  (GTINTEGER_t *this, BOOL value);

/***********************************************************************************
 *
 * GTNUMBER
 *
 ***********************************************************************************/

typedef struct GTNUMBER_ GTNUMBER_t;

struct GTNUMBER_
{
    GTGADGET_t      gadget;
    LONG            number;
    BOOL            border;
    UBYTE           frontPen;
    UBYTE           backPen;
    UBYTE           justification;
    CONST_STRPTR    format;
    ULONG           maxNumberLen;
    BOOL            clipped;
};

void _GTNUMBER_CONSTRUCTOR (GTNUMBER_t *this,
                            CONST_STRPTR label, LONG number,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

LONG                   _GTNUMBER_number_ (GTNUMBER_t *this);
void                   _GTNUMBER_number  (GTNUMBER_t *this, LONG value);

BOOL                   _GTNUMBER_border_ (GTNUMBER_t *this);
void                   _GTNUMBER_border  (GTNUMBER_t *this, BOOL value);

UBYTE                  _GTNUMBER_frontPen_ (GTNUMBER_t *this);
void                   _GTNUMBER_frontPen  (GTNUMBER_t *this, UBYTE value);

UBYTE                  _GTNUMBER_backPen_ (GTNUMBER_t *this);
void                   _GTNUMBER_backPen  (GTNUMBER_t *this, UBYTE value);

UBYTE                  _GTNUMBER_justification_ (GTNUMBER_t *this);
void                   _GTNUMBER_justification  (GTNUMBER_t *this, UBYTE value);

CONST_STRPTR           _GTNUMBER_format_ (GTNUMBER_t *this);
void                   _GTNUMBER_format  (GTNUMBER_t *this, CONST_STRPTR value);

ULONG                  _GTNUMBER_maxNumberLen_ (GTNUMBER_t *this);
void                   _GTNUMBER_maxNumberLen  (GTNUMBER_t *this, ULONG value);

BOOL                   _GTNUMBER_clipped_ (GTNUMBER_t *this);
void                   _GTNUMBER_clipped  (GTNUMBER_t *this, BOOL value);


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
    GTGADGET_t        *first;
    GTGADGET_t        *last;
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

