#ifndef HAVE_GADTOOLS_SUPPORT_H
#define HAVE_GADTOOLS_SUPPORT_H

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

//void        GTG_MODIFY         (GTGADGET_t *g, ULONG ti_Tag, ...);

//BOOL        GTGSELECTED_       (GTGADGET_t *g);
//STRPTR      GTGBUFFER_         (GTGADGET_t *g);
//LONG        GTGNUM_            (GTGADGET_t *g);

void        GTGADGETS_DEPLOY   (void);
void        GTGADGETS_FREE     (void);

void        GTG_DRAW_BEVEL_BOX (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, BOOL recessed );

//void        ON_GTG_UP_CALL     (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data);
//void        ON_GTG_DOWN_CALL   (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data);
//void        ON_GTG_MOVE_CALL   (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data);

SHORT _GTGADGET_NEXT_ID (void);

#endif

