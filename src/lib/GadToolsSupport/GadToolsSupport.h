#ifndef HAVE_UI_SUPPORT_H
#define HAVE_UI_SUPPORT_H

#define AE_GTG_CREATE   400
#define AE_GTG_MODIFY   401
#define AE_GTG_DEPLOY   402
#define AE_GTG_SELECTED 403
#define AE_GTG_CALLBACK 404
#define AE_GTG_BUFFER   405
#define AE_GTG_NUM      406

typedef struct GTGADGET_ GTGADGET_t;

typedef void (*gtgadget_cb_t)(SHORT wid, SHORT gid, USHORT code, void* user_data);

struct GTGADGET_
{
    GTGADGET_t       *prev, *next;

    SHORT             id;
    struct Gadget    *gad;
    struct Window    *win;

    gtgadget_cb_t     gadgetup_cb;
    void             *gadgetup_user_data;
    gtgadget_cb_t     gadgetdown_cb;
    void             *gadgetdown_user_data;
    gtgadget_cb_t     gadgetmove_cb;
    void             *gadgetmove_user_data;
};

GTGADGET_t *GTGADGET_        (SHORT kind,
                              BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                              char *txt, ULONG flags, SHORT id, ULONG ti_Tag, ...);

void        GTG_MODIFY       (GTGADGET_t *g, ULONG ti_Tag, ...);

BOOL        GTGSELECTED_     (GTGADGET_t *g);
STRPTR      GTGBUFFER_       (GTGADGET_t *g);
LONG        GTGNUM_          (GTGADGET_t *g);

void        GTGADGETS_DEPLOY (void);
void        GTGADGETS_FREE   (void);

void        ON_GTG_UP_CALL   (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data);
void        ON_GTG_DOWN_CALL (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data);
void        ON_GTG_MOVE_CALL (GTGADGET_t *g, gtgadget_cb_t cb, void *user_data);

#endif

