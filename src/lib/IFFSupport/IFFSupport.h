#ifndef HAVE_IFF_SUPPORT_H
#define HAVE_IFF_SUPPORT_H

#include "_aqb.h"

/*
 * IFF / ILBM support
 */

#define AE_IFF                      200

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

void ILBM_LOAD (USHORT fno, ILBM_META_t *pMeta, PALETTE_t pPalette, BlitNode blt);

#endif

