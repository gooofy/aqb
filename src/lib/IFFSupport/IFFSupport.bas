' IFF / ILBM support

PUBLIC CONST AS INTEGER AE_IFF                = 122

PUBLIC TYPE ILBM_BitMapHeader_t
    AS UINTEGER    w, h                   : REM raster width & height in pixels
    AS INTEGER     x, y                   : REM position for this image
    AS UBYTE       nPlanes                : REM # source bitplanes
    AS UBYTE       masking                : REM masking technique
    AS UBYTE       compression            : REM compression algorithm
    AS UBYTE       pad1                   : REM UNUSED.  For consistency, put 0 here.
    AS UINTEGER    transparentColor       : REM transparent "color number"
    AS UBYTE       xAspect, yAspect       : REM aspect ratio, a rational number x/y
    AS INTEGER     pageWidth, pageHeight  : REM source "page" size in pixels
END TYPE

PUBLIC DECLARE SUB ILBM LOAD BMHD (_FNO(BYVAL fno AS UINTEGER), BYVAL pBMHD AS ILBM_BitMapHeader_t PTR)
PUBLIC DECLARE SUB ILBM LOAD CMAP (_FNO(BYVAL fno AS UINTEGER), BYVAL pPalette AS PALETTE_t PTR)
PUBLIC DECLARE SUB ILBM LOAD BODY (_FNO(BYVAL fno AS UINTEGER), BYVAL pBMHD AS ILBM_BitMapHeader_t PTR, BYVAL blit AS VOID PTR)


