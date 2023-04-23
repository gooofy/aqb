'
' gfx library SPRITEs and GELs support
'

IMPORT IFFSupport

OPTION EXPLICIT
OPTION PRIVATE

' --------------------------------------------------------------------------------------------------------
' --
' -- ERROR codes for AnimSupport 3xx
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC CONST AS INTEGER AE_BOB                = 300
PUBLIC CONST AS INTEGER AE_SPRITE             = 301

' sprites

PUBLIC TYPE SPRITE_t
    AS SPRITE_t PTR     prev, next
    AS ANY PTR          posctldata
    AS UINTEGER         width
    AS UINTEGER         height
END TYPE

PUBLIC DECLARE EXTERN FUNCTION SPRITE           (BYVAL bm AS BITMAP_t PTR, _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=0, BYVAL y1 AS INTEGER=0, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1)) AS SPRITE_t PTR
PUBLIC DECLARE EXTERN SUB      SPRITE SHOW      (BYVAL spnum AS INTEGER, BYVAL sprite AS SPRITE_t PTR)
PUBLIC DECLARE EXTERN SUB      SPRITE HIDE      (BYVAL spnum AS INTEGER)
PUBLIC DECLARE EXTERN SUB      SPRITE MOVE      (BYVAL spnum AS INTEGER, _COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER=-1, BYVAL y AS INTEGER=-1))
PUBLIC DECLARE EXTERN SUB      SPRITE FREE      (BYVAL sprite AS SPRITE_t PTR)

PUBLIC DECLARE EXTERN SUB      ILBM LOAD SPRITE (BYVAL path AS STRING, _
                                                 BYREF sprite AS SPRITE_t PTR, _
                                                 BYVAL scid AS INTEGER=-1, _
                                                 BYVAL pMETA AS ILBM_META_t PTR = NULL, _
                                                 BYVAL pPalette AS PALETTE_t PTR = NULL)

' custom mouse pointers

PUBLIC DECLARE EXTERN SUB      POINTER SPRITE   (BYVAL sprite AS SPRITE_t PTR)
PUBLIC DECLARE EXTERN SUB      POINTER CLEAR

' GELS

PUBLIC DECLARE EXTERN SUB      GELS INIT        (BYVAL sprRsrvd AS UBYTE = 0)
PUBLIC DECLARE EXTERN SUB      GELS REPAINT

' bobs

PUBLIC TYPE BOB_t
    AS BOB_t PTR        prev, next
    AS ANY PTR          rp
    ' FIXME: struct Bob      bob
    ' FIXME: struct VSprite  vsprite
END TYPE

PUBLIC DECLARE EXTERN FUNCTION BOB              (BYVAL bm AS BITMAP_t PTR, _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=0, BYVAL y1 AS INTEGER=0, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1)) AS BOB_t PTR
PUBLIC DECLARE EXTERN SUB      BOB MOVE         (BYVAL bob AS BOB_t PTR, _COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER=-1, BYVAL y AS INTEGER=-1))
PUBLIC DECLARE EXTERN SUB      BOB SHOW         (BYVAL bob AS BOB_t PTR)
PUBLIC DECLARE EXTERN SUB      BOB HIDE         (BYVAL bob AS BOB_t PTR)
PUBLIC DECLARE EXTERN SUB      BOB FREE         (BYVAL bob AS BOB_t PTR)

PUBLIC DECLARE EXTERN SUB      ILBM LOAD BOB    (BYVAL path AS STRING, _
                                                 BYREF bob AS BOB_t PTR, _
                                                 BYVAL scid AS INTEGER=-1, _
                                                 BYVAL pMETA AS ILBM_META_t PTR = NULL, _
                                                 BYVAL pPalette AS PALETTE_t PTR = NULL)

