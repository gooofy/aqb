'
' gfx library SPRITEs and GELs support
'

IMPORT IFFSupport

OPTION EXPLICIT
OPTION PRIVATE

PUBLIC CONST AS INTEGER AE_BOB                = 300
PUBLIC CONST AS INTEGER AE_SPRITE             = 301

' sprites

PUBLIC TYPE SPRITE_t
    AS SPRITE_t PTR     prev, next
    AS VOID PTR         posctldata
    AS UINTEGER         width
    AS UINTEGER         height
END TYPE

PUBLIC DECLARE FUNCTION SPRITE           (BYVAL bm AS BITMAP_t PTR, _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=0, BYVAL y1 AS INTEGER=0, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1)) AS SPRITE_t PTR
PUBLIC DECLARE SUB      SPRITE SHOW      (BYVAL spnum AS INTEGER, BYVAL sprite AS SPRITE_t PTR)
PUBLIC DECLARE SUB      SPRITE HIDE      (BYVAL spnum AS INTEGER)
PUBLIC DECLARE SUB      SPRITE MOVE      (BYVAL spnum AS INTEGER, _COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER=-1, BYVAL y AS INTEGER=-1))
PUBLIC DECLARE SUB      SPRITE FREE      (BYVAL sprite AS SPRITE_t PTR)

PUBLIC DECLARE SUB      ILBM LOAD SPRITE (BYVAL path AS STRING, _
                                          BYREF sprite AS SPRITE_t PTR, _
                                          BYVAL scid AS INTEGER=-1, _
                                          BYVAL pMETA AS ILBM_META_t PTR = NULL, _
                                          BYVAL pPalette AS PALETTE_t PTR = NULL)

' custom mouse pointers

PUBLIC DECLARE SUB      POINTER SPRITE   (BYVAL sprite AS SPRITE_t PTR)
PUBLIC DECLARE SUB      POINTER CLEAR

' GELS

PUBLIC DECLARE SUB      GELS INIT        (BYVAL sprRsrvd AS UBYTE = 0)
PUBLIC DECLARE SUB      GELS REPAINT

' bobs

PUBLIC TYPE BOB_t
    AS BOB_t PTR        prev, next
    AS VOID PTR         rp
    ' FIXME: struct Bob      bob
    ' FIXME: struct VSprite  vsprite
END TYPE

PUBLIC DECLARE FUNCTION BOB              (BYVAL bm AS BITMAP_t PTR, _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=0, BYVAL y1 AS INTEGER=0, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1)) AS BOB_t PTR
PUBLIC DECLARE SUB      BOB MOVE         (BYVAL bob AS BOB_t PTR, _COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER=-1, BYVAL y AS INTEGER=-1))
PUBLIC DECLARE SUB      BOB SHOW         (BYVAL bob AS BOB_t PTR)
PUBLIC DECLARE SUB      BOB HIDE         (BYVAL bob AS BOB_t PTR)
PUBLIC DECLARE SUB      BOB FREE         (BYVAL bob AS BOB_t PTR)

PUBLIC DECLARE SUB      ILBM LOAD BOB    (BYVAL path AS STRING, _
                                          BYREF bob AS BOB_t PTR, _
                                          BYVAL scid AS INTEGER=-1, _
                                          BYVAL pMETA AS ILBM_META_t PTR = NULL, _
                                          BYVAL pPalette AS PALETTE_t PTR = NULL)

