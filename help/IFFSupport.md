
# AQB / IFFSupport

:toc:

## ILBM LOAD BITMAP

Syntax:

	ILBM LOAD BITMAP path [, [bmPtr] [, [scid] [, [pMETA] [, [pPalette] [, [cont] ] ] ] ] ]

load a bitmap from an IFF ILBM file

Arguments:
 * path: pathname of the file to load the bitmap from
 * bmPtr: reference to a bitmap pointer, image data will be stored in
             this bitmap. If the referenced pointer is NULL, a bitmap will
             be allocated for you.
 * scid: screen id - if given, colors will be applied to it
 * pMETA: pointer to metadata
 * pPalette: pointer to a palette
 * cont: if TRUE, allocate bitplanes in a continuous memory area suitable for BOBs

Example:

	DIM AS BITMAP_t PTR dragon = NULL

	ILBM LOAD BITMAP "PROGDIR:imgs/dragon.iff", dragon, 1

## ILBM READ BITMAP

Syntax:

	ILBM READ BITMAP fno, [, [bmPtr] [, [scid] [, [pMETA] [, [pPalette] [, [cont] ] ] ] ] ]

load a bitmap from an IFF ILBM file stream. See ILBM LOAD BITMAP for details.

## ILBM_META_t

Structure of IFF ILBM image meta data information:

	PUBLIC TYPE ILBM_META_t

		REM BMHD

		AS UINTEGER    w, h                   : REM raster width & height in pixels
		AS INTEGER     x, y                   : REM position for this image
		AS UBYTE       nPlanes                : REM # source bitplanes
		AS UBYTE       masking                : REM masking technique
		AS UBYTE       compression            : REM compression algorithm
		AS UBYTE       pad1                   : REM UNUSED.  For consistency, put 0 here.
		AS UINTEGER    transparentColor       : REM transparent "color number"
		AS UBYTE       xAspect, yAspect       : REM aspect ratio, a rational number x/y
		AS INTEGER     pageWidth, pageHeight  : REM source "page" size in pixels

		REM CAMG (optional)

		AS ULONG       viewModes

	END TYPE


## IFF8SVX LOAD WAVE

Syntax:

    IFF8SVX LOAD WAVE path "," w

load an instrument from an IFF 8SVX file

Arguments:
 * path: pathname of the file to load the instrument from
 * w: reference to a WAVE_t pointer

Example:

    DIM AS WAVE_t PTR w = NULL
    IFF8SVX LOAD WAVE "PROGDIR:/8svx/BassGt.8svx", w


## IFF8SVX READ WAVE

Syntax:

    IFF8SVX READ WAVE fno "," w

load an instrument from an IFF 8SVX file stream. see IFF8SVX LOAD WAVE for details.


