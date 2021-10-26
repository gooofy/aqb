
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
    * cont: if TRUE, allocate bitplanes in a continous memory area suitable for BOBs

Example:

	DIM AS BITMAP\_t PTR dragon = NULL

	ILBM LOAD BITMAP "PROGDIR:imgs/dragon.iff", dragon, 1

## ILBM READ BITMAP

Syntax:

	ILBM READ BITMAP fno, [, [bmPtr] [, [scid] [, [pMETA] [, [pPalette] [, [cont] ] ] ] ] ]

load a bitmap from an IFF ILBM file stream. See ILBM LOAD BITMAP for details.

## ILBM READ BOB

Syntax:

	ILBM LOAD BOB path [, [bobPtr] [, [scid] [, [pMETA] [, [pPalette] ] ] ] ]

load a bob from an IFF ILBM file. See ILBM LOAD BITMAP for details.

Example:

	DIM AS BOB\_t PTR dragon = NULL

	ILBM LOAD BOB "PROGDIR:imgs/dragon.iff", bob, 1


## ILBM\_META\_t

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

