'
' gfx library GELs support
'

IMPORT IFFSupport

OPTION EXPLICIT
OPTION PRIVATE

PUBLIC CONST AS INTEGER AE_BOB                = 300

' GELS

PUBLIC DECLARE SUB GELS INIT            (BYVAL sprRsrvd AS UBYTE = 0)

PUBLIC DECLARE SUB GELS REPAINT


' sprites

' vsprites

' bobs

PUBLIC TYPE BOB_t
    AS BOB_t PTR        prev, next
    AS BOOLEAN          active
    ' FIXME: struct Bob      bob
    ' FIXME: struct VSprite  vsprite
END TYPE

PUBLIC DECLARE FUNCTION BOB             (BYVAL bm AS BITMAP_t PTR) AS BOB_t PTR
PUBLIC DECLARE SUB      BOB MOVE        (BYVAL bob AS BOB_t PTR, _COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER=-1, BYVAL y AS INTEGER=-1))

PUBLIC DECLARE SUB      ILBM LOAD BOB   (BYVAL path AS STRING, _
                                         BYREF bob AS BOB_t PTR, _
                                         BYVAL scid AS INTEGER=-1, _
                                         BYVAL pMETA AS ILBM_META_t PTR = NULL, _
                                         BYVAL pPalette AS PALETTE_t PTR = NULL)

