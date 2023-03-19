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
typedef struct GTLAYOUT_   GTLAYOUT_t;
typedef struct GTGADGET_   GTGADGET_t;

typedef void (*gtgadget_cb_t)(GTGADGET_t *gtg, USHORT code);
typedef struct Gadget * (*gtgadget_deploy_cb_t)(GTGADGET_t *gtg, struct Gadget *gad, APTR vinfo, struct TextAttr *ta,
                                                SHORT x, SHORT y, SHORT w, SHORT h);
typedef void (*gtgadget_add_child_cb_t)(GTGADGET_t *gtg, GTGADGET_t *child);

#define GT_DOMAIN_MINIMUM     0
#define GT_DOMAIN_NOMINAL     1
#define GT_DOMAIN_MAXIMUM     2
typedef void (*gtgadget_domain_cb_t)(GTGADGET_t *gtg, SHORT which, SHORT *w, SHORT *h);

struct GTGADGET_
{
    gtgadget_cb_t           gadgetup_cb;
    gtgadget_cb_t           gadgetdown_cb;
    gtgadget_cb_t           gadgetmove_cb;

    void                   *user_data;
    ULONG                   underscore;

    GTGADGET_t             *prev, *next;

    struct NewGadget        ng;

    struct Gadget          *gad;
    SHORT                   win_id;

    gtgadget_deploy_cb_t    deploy_cb;
    gtgadget_add_child_cb_t add_child_cb;
    gtgadget_domain_cb_t    domain_cb;
};

void _GTGADGET_CONSTRUCTOR (GTGADGET_t *this, GTGADGET_t *parent,
                            char *txt, SHORT id,
                            void *user_data, ULONG flags, ULONG underscore);

// GTGADGET properties
CONST_STRPTR _GTGADGET_text_      (GTGADGET_t *this);
void         _GTGADGET_text       (GTGADGET_t *this, STRPTR text);
SHORT        _GTGADGET_id_        (GTGADGET_t *this);
void         _GTGADGET_id         (GTGADGET_t *this, SHORT id);
ULONG        _GTGADGET_flags_     (GTGADGET_t *this);
void         _GTGADGET_flags      (GTGADGET_t *this, ULONG flags);
BOOL         _GTGADGET_deployed_  (GTGADGET_t *this);

struct GTLAYOUT_
{
    GTGADGET_t      gadget;
    BOOL            horiz;
    GTGADGET_t     *child_first, *child_last;
};

void _GTLAYOUT_CONSTRUCTOR (GTLAYOUT_t *this, GTGADGET_t *parent, BOOL horiz);

struct GTBUTTON_
{
    GTGADGET_t      gadget;
    BOOL            disabled;
};

void _GTBUTTON_CONSTRUCTOR (GTBUTTON_t *this, GTGADGET_t *parent,
                            char *txt, SHORT id,
                            void *user_data, ULONG flags, ULONG underscore);

// GTBUTTON properties
BOOL _GTBUTTON_disabled_ (GTBUTTON_t *this);
void _GTBUTTON_disabled  (GTBUTTON_t *this, BOOL disabled);


void        GTGADGETS_DEPLOY   (void);
void        GTGADGETS_FREE     (void);

void        GTG_DRAW_BEVEL_BOX (BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2, BOOL recessed );

#endif

