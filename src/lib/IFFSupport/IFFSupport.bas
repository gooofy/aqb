' IFF / ILBM support

PUBLIC CONST AS INTEGER AE_IFF = 200

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

PUBLIC DECLARE SUB ILBM LOAD (_FNO(BYVAL fno AS UINTEGER), BYVAL pMETA AS ILBM_META_t PTR, BYVAL pPalette AS PALETTE_t PTR = NULL, BYVAL bm AS BITMAP_t PTR = NULL)

