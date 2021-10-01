#ifndef HAVE_IFF_SUPPORT_H
#define HAVE_IFF_SUPPORT_H

#include "_aqb.h"

/*
 * IFF / ILBM support
 */

#define AE_IFF                      200

/* A BitMapHeader is stored in a BMHD chunk. */
typedef struct
{
    UWORD       w, h;                   /* raster width & height in pixels */
    WORD        x, y;                   /* position for this image */
    UBYTE       nPlanes;                /* # source bitplanes */
    UBYTE       masking;                /* masking technique */
    UBYTE       compression;            /* compression algorithm */
    UBYTE       pad1;                   /* UNUSED.  For consistency, put 0 here.*/
    UWORD       transparentColor;       /* transparent "color number" */
    UBYTE       xAspect, yAspect;       /* aspect ratio, a rational number x/y */
    WORD        pageWidth, pageHeight;  /* source "page" size in pixels */
} ILBM_BitMapHeader_t;

void     ILBM_LOAD_BMHD       (USHORT fno, ILBM_BitMapHeader_t *pBMHD);
void     ILBM_LOAD_BODY       (USHORT fno, ILBM_BitMapHeader_t *pBMHD, BlitNode blit);

#endif

