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

typedef struct GTBUTTON_   GTBUTTON_t;
typedef struct GTCHECKBOX_ GTCHECKBOX_t;
typedef struct GTSLIDER_   GTSLIDER_t;
typedef struct GTGADGET_   GTGADGET_t;

/***********************************************************************************
 *
 * GTGADGET
 *
 ***********************************************************************************/

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

void _GTGADGET_CONSTRUCTOR (GTGADGET_t *this, char *txt,
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

struct GTBUTTON_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
};

void _GTBUTTON_CONSTRUCTOR (GTBUTTON_t *this, char *txt,
                            BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                            void *user_data, ULONG flags, ULONG underscore);

// GTBUTTON properties
BOOL _GTBUTTON_disabled_ (GTBUTTON_t *this);
void _GTBUTTON_disabled  (GTBUTTON_t *this, BOOL disabled);

/***********************************************************************************
 *
 * GTCHECKBOX
 *
 ***********************************************************************************/

struct GTCHECKBOX_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
    BOOL            checked;
};

void _GTCHECKBOX_CONSTRUCTOR (GTCHECKBOX_t *this, char *txt,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              void *user_data, ULONG flags, ULONG underscore);

// GTCHECKBOX properties
BOOL _GTCHECKBOX_disabled_ (GTCHECKBOX_t *this);
void _GTCHECKBOX_disabled  (GTCHECKBOX_t *this, BOOL disabled);
BOOL _GTCHECKBOX_checked_  (GTCHECKBOX_t *this);
void _GTCHECKBOX_checked   (GTCHECKBOX_t *this, BOOL checked);

/***********************************************************************************
 *
 * GTSLIDER
 *
 ***********************************************************************************/

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

void _GTSLIDER_CONSTRUCTOR (GTSLIDER_t *this,
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

