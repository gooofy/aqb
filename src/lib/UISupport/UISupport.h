#ifndef HAVE_UI_SUPPORT_H
#define HAVE_UI_SUPPORT_H

#define AE_GTGADGET_CREATE  400
#define AE_GTGADGET_DEPLOY  401

typedef struct GTGADGET_ GTGADGET_t;

typedef void (*gtgadgetup_cb_t)(SHORT wid, SHORT gid, GTGADGET_t *g);

struct GTGADGET_
{
    GTGADGET_t       *prev, *next;

    SHORT             id;
    gtgadgetup_cb_t   gadgetup_cb;
};

GTGADGET_t *GTGADGET_ (SHORT kind,
                       BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2,
                       char *txt, ULONG flags, SHORT id, ULONG ti_Tag, ...);

void GTGADGETS_DEPLOY (void);
void GTGADGETS_FREE   (void);

void ON_GTGADGETUP_CALL (GTGADGET_t *g, gtgadgetup_cb_t cb);

#endif

