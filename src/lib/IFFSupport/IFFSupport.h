#ifndef HAVE_IFF_SUPPORT_H
#define HAVE_IFF_SUPPORT_H

#include "_aqb.h"

/*
 * IFF / ILBM support
 */

#define AE_IFF                      200

#define mskNone                0
#define mskHasMask             1
#define mskHasTransparentColor 2
#define mskLasso               3

typedef struct
{
    // BitMapHeader (BMHD chunk)

    UWORD       w, h;                   // raster width & height in pixels
    WORD        x, y;                   // position for this image
    UBYTE       nPlanes;                // # source bitplanes
    UBYTE       masking;                // masking technique
    UBYTE       compression;            // compression algorithm
    UBYTE       pad1;                   // UNUSED.  For consistency, put 0 here.
    UWORD       transparentColor;       // transparent "color number"
    UBYTE       xAspect, yAspect;       // aspect ratio, a rational number x/y
    WORD        pageWidth, pageHeight;  // source "page" size in pixels

    // CAMG (optional)

    ULONG       viewMode;

} ILBM_META_t;

void ILBM_LOAD_BITMAP (STRPTR path, BITMAP_t **bm, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette, BOOL cont);

void ILBM_READ_BITMAP (USHORT fno, BITMAP_t **bm, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette, BOOL cont);

#endif

