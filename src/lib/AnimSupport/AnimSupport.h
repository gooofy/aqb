#ifndef HAVE_ANIM_SUPPORT_H
#define HAVE_ANIM_SUPPORT_H

#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <graphics/gels.h>

#include "../_brt/_brt.h"
#include "../_aqb/_aqb.h"
#include "../IFFSupport/IFFSupport.h"

#define AE_GELS_INIT                300
#define AE_BOB                      301
#define AE_SPRITE                   302

/*
 * SPRITEs
 */

typedef struct SPRITE_ SPRITE_t;

struct SPRITE_
{
    SPRITE_t *prev, *next;
    UWORD    *posctldata;
    UWORD     width, height;
};

SPRITE_t *SPRITE_               (BITMAP_t *bm, BOOL s1, SHORT x1, SHORT y1, BOOL s2, SHORT x2, SHORT y2);
void      SPRITE_SHOW           (SHORT spnum, SPRITE_t *sprite);
void      SPRITE_HIDE           (SHORT spnum);
void      SPRITE_MOVE           (SHORT spnum, BOOL s, SHORT x, SHORT y);
void      SPRITE_FREE           (SPRITE_t *sprite);

void      ILBM_LOAD_SPRITE      (STRPTR path, SPRITE_t **sprite, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette);

/*
 * custom mouse pointers
 */

void      POINTER_SPRITE        (SPRITE_t *sprite, SHORT xoffset, SHORT yoffset);
void      POINTER_CLEAR         (void);

/*
 * GELs
 */

void GELS_INIT      (UBYTE sprRsrvd);
void GELS_REPAINT   (void);

/*
 * BOBs
 */

typedef struct BOB_ BOB_t;

struct BOB_
{
    BOB_t           *prev, *next;
    struct RastPort *rp;
    struct Bob       bob;
    struct VSprite   vsprite;
};

BOB_t *BOB_               (BITMAP_t *bm);
void   BOB_MOVE           (BOB_t *bob, BOOL s, SHORT x, SHORT y);
void   BOB_HIDE           (BOB_t *bob);
void   BOB_FREE           (BOB_t *bob);

void   ILBM_LOAD_BOB      (STRPTR path, BOB_t **bob, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette);

#endif

