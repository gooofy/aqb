'
' _AQB module interface
'

OPTION EXPLICIT
OPTION PRIVATE

IMPORT OSExec
IMPORT OSUtility

' --------------------------------------------------------------------------------------------------------
' --
' -- ERROR codes for _aqb 1xx
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC CONST AS INTEGER AE_WIN_OPEN          = 101
PUBLIC CONST AS INTEGER AE_SCREEN_OPEN       = 102
PUBLIC CONST AS INTEGER AE_PALETTE           = 103
PUBLIC CONST AS INTEGER AE_COLOR             = 104
PUBLIC CONST AS INTEGER AE_AREA              = 105
PUBLIC CONST AS INTEGER AE_PATTERN           = 106
PUBLIC CONST AS INTEGER AE_WIN_CLOSE         = 107
PUBLIC CONST AS INTEGER AE_WIN_OUTPUT        = 108
PUBLIC CONST AS INTEGER AE_SCREEN_CLOSE      = 109
PUBLIC CONST AS INTEGER AE_PAINT             = 110
PUBLIC CONST AS INTEGER AE_LINE              = 111
PUBLIC CONST AS INTEGER AE_PSET              = 112
PUBLIC CONST AS INTEGER AE_ON_TIMER_CALL     = 114
PUBLIC CONST AS INTEGER AE_TIMER_ON          = 115
PUBLIC CONST AS INTEGER AE_TIMER_OFF         = 116
PUBLIC CONST AS INTEGER AE_MOUSE             = 120
PUBLIC CONST AS INTEGER AE_BLIT              = 121
PUBLIC CONST AS INTEGER AE_RASTPORT          = 122
PUBLIC CONST AS INTEGER AE_FONT              = 123
PUBLIC CONST AS INTEGER AE_AUDIO             = 124
PUBLIC CONST AS INTEGER AE_WIN_CALL          = 125
PUBLIC CONST AS INTEGER AE_EXEC_LIST         = 126

' --------------------------------------------------------------------------------------------------------
' --
' -- tags
' --
' --------------------------------------------------------------------------------------------------------

PUBLIC DECLARE EXTERN FUNCTION TAGITEMS     (BYVAL ti_Tag AS ULONG, ...) AS TAGITEM PTR
PUBLIC DECLARE EXTERN FUNCTION TAGS         (BYVAL ti_Tag AS ULONG, ...) AS ULONG PTR

' --------------------------------------------------------------------------------------------------------
' --
' -- bitmaps, screens, windows and graphics
' --
' --------------------------------------------------------------------------------------------------------

' graphical text output

PUBLIC DECLARE EXTERN SUB      LOCATE XY (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER=-1, BYVAL y AS INTEGER=-1))
PUBLIC DECLARE EXTERN FUNCTION CSRLIN    () AS INTEGER
PUBLIC DECLARE EXTERN FUNCTION POS       (BYVAL dummy AS INTEGER) AS INTEGER

' colors and palettes

PUBLIC TYPE COLOR_t
    AS UBYTE    r, g, b
END TYPE

PUBLIC TYPE PALETTE_t
    AS INTEGER  numEntries
    AS COLOR_t  colors(256)
END TYPE

PUBLIC DECLARE EXTERN SUB      PALETTE         (BYVAL cid AS INTEGER, BYVAL red AS SINGLE, BYVAL green AS SINGLE, BYVAL blue AS SINGLE)
PUBLIC DECLARE EXTERN SUB      PALETTE LOAD    (BYVAL p AS PALETTE_t PTR)

PUBLIC DECLARE EXTERN FUNCTION INKEY$          ()

' BitMaps

PUBLIC TYPE BITMAP_t
    AS BITMAP_t PTR     prev, next
    AS INTEGER          width, height
    AS BOOLEAN          cont
    AS ANY PTR          MASK
    ' FIXME struct BitMap   bm
    ' FIXME struct RastPort rp
END TYPE

PUBLIC DECLARE EXTERN FUNCTION BITMAP          (BYVAL width AS INTEGER, BYVAL height AS INTEGER, BYVAL depth AS INTEGER, BYVAL cont AS BOOLEAN=FALSE) AS BITMAP_t PTR
PUBLIC DECLARE EXTERN SUB      BITMAP FREE     (BYVAL bm AS BITMAP_t PTR)
PUBLIC DECLARE EXTERN SUB      BITMAP OUTPUT   (BYVAL bm AS BITMAP_t PTR)
PUBLIC DECLARE EXTERN SUB      BITMAP MASK     (BYVAL bm AS BITMAP_t PTR)
PUBLIC DECLARE EXTERN SUB      GET             (_COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=-1, BYVAL y1 AS INTEGER=-1, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1), BYVAL blit AS ANY PTR )
PUBLIC DECLARE EXTERN SUB      PUT             (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER=-1, BYVAL y AS INTEGER=-1), BYVAL blit AS ANY PTR , BYVAL minterm AS UBYTE=&HC0, _
                                         _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=-1, BYVAL y1 AS INTEGER=-1, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1))

' fonts

PUBLIC TYPE FONT_t
    AS FONT_t PTR       prev, next
    AS ANY PTR          dfh
    AS ANY PTR          tf
END TYPE

PUBLIC DECLARE EXTERN FUNCTION FONT          (BYVAL name AS STRING, BYVAL size AS INTEGER, BYVAL fontdir AS STRING = NULL) AS FONT_t PTR
PUBLIC DECLARE EXTERN SUB      FONT FREE     (BYVAL font AS FONT_t PTR)
PUBLIC DECLARE EXTERN SUB      FONT          (BYVAL font AS FONT_t PTR)

PUBLIC CONST AS ULONG FSF_UNDERLINED = 1
PUBLIC CONST AS ULONG FSF_BOLD       = 2
PUBLIC CONST AS ULONG FSF_ITALIC     = 4
PUBLIC CONST AS ULONG FSF_EXTENDED   = 8

PUBLIC DECLARE EXTERN SUB      FONTSTYLE     (BYVAL style AS ULONG)
PUBLIC DECLARE EXTERN FUNCTION FONTSTYLE     AS ULONG

PUBLIC DECLARE EXTERN FUNCTION TEXTWIDTH     (BYVAL s AS STRING) AS INTEGER
PUBLIC DECLARE EXTERN SUB      TEXTEXTEND    (BYVAL s AS STRING, BYREF w AS INTEGER, BYREF h AS INTEGER)

' screens

PUBLIC CONST AS UINTEGER AS_MODE_GENLOCK_VIDEO   = &H0002
PUBLIC CONST AS UINTEGER AS_MODE_LACE            = &H0004
PUBLIC CONST AS UINTEGER AS_MODE_DOUBLESCAN      = &H0008
PUBLIC CONST AS UINTEGER AS_MODE_SUPERHIRES      = &H0020
PUBLIC CONST AS UINTEGER AS_MODE_PFBA            = &H0040
PUBLIC CONST AS UINTEGER AS_MODE_EXTRA_HALFBRITE = &H0080
PUBLIC CONST AS UINTEGER AS_MODE_GENLOCK_AUDIO   = &H0100
PUBLIC CONST AS UINTEGER AS_MODE_DUALPF          = &H0400
PUBLIC CONST AS UINTEGER AS_MODE_HAM             = &H0800
PUBLIC CONST AS UINTEGER AS_MODE_EXTENDED_MODE   = &H1000
PUBLIC CONST AS UINTEGER AS_MODE_VP_HIDE         = &H2000
PUBLIC CONST AS UINTEGER AS_MODE_SPRITES         = &H4000
PUBLIC CONST AS UINTEGER AS_MODE_HIRES           = &H8000

PUBLIC DECLARE EXTERN SUB SCREEN (BYVAL id AS INTEGER, BYVAL width AS INTEGER, BYVAL height AS INTEGER, BYVAL depth AS INTEGER, BYVAL mode AS UINTEGER, BYVAL title AS STRING=NULL, BYVAL bm AS BITMAP_t PTR=NULL)
PUBLIC DECLARE EXTERN SUB SCREEN CLOSE (BYVAL id AS INTEGER)

' windows

PUBLIC CONST AS ULONG AW_FLAG_SIZEGADGET     = &H00000001
PUBLIC CONST AS ULONG AW_FLAG_DRAGBAR        = &H00000002
PUBLIC CONST AS ULONG AW_FLAG_DEPTHGADGET    = &H00000004
PUBLIC CONST AS ULONG AW_FLAG_CLOSEGADGET    = &H00000008
PUBLIC CONST AS ULONG AW_FLAG_SIZEBRIGHT     = &H00000010
PUBLIC CONST AS ULONG AW_FLAG_SIZEBBOTTOM    = &H00000020
PUBLIC CONST AS ULONG AW_FLAG_REFRESHBITS    = &H000000C0
PUBLIC CONST AS ULONG AW_FLAG_SMART_REFRESH  = &H00000000
PUBLIC CONST AS ULONG AW_FLAG_SIMPLE_REFRESH = &H00000040
PUBLIC CONST AS ULONG AW_FLAG_SUPER_BITMAP   = &H00000080
PUBLIC CONST AS ULONG AW_FLAG_OTHER_REFRESH  = &H000000C0
PUBLIC CONST AS ULONG AW_FLAG_BACKDROP       = &H00000100
PUBLIC CONST AS ULONG AW_FLAG_REPORTMOUSE    = &H00000200
PUBLIC CONST AS ULONG AW_FLAG_GIMMEZEROZERO  = &H00000400
PUBLIC CONST AS ULONG AW_FLAG_BORDERLESS     = &H00000800
PUBLIC CONST AS ULONG AW_FLAG_ACTIVATE       = &H00001000
PUBLIC CONST AS ULONG AW_FLAG_RMBTRAP        = &H00010000
PUBLIC CONST AS ULONG AW_FLAG_NOCAREREFRESH  = &H00020000
PUBLIC CONST AS ULONG AW_FLAG_NW_EXTENDED    = &H00040000
PUBLIC CONST AS ULONG AW_FLAG_NEWLOOKMENUS   = &H00200000

' WINDOW id [, [Title] [, [(x1,y1)-(x2,y2)] [, [Flags] [, Screen] ] ]
PUBLIC DECLARE EXTERN SUB      WINDOW (BYVAL wid AS INTEGER, BYVAL title AS STRING = NULL, _
                                       _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=-1, BYVAL y1 AS INTEGER=-1, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1), _
                                       BYVAL flags AS ULONG=AW_FLAG_SIZEGADGET OR AW_FLAG_DRAGBAR OR AW_FLAG_DEPTHGADGET OR AW_FLAG_CLOSEGADGET OR AW_FLAG_SMART_REFRESH OR AW_FLAG_GIMMEZEROZERO OR AW_FLAG_ACTIVATE, _
                                       BYVAL scrid AS INTEGER = 0)

PUBLIC DECLARE EXTERN SUB      WINDOW CLOSE           (BYVAL wid AS INTEGER = 0)
PUBLIC DECLARE EXTERN SUB      WINDOW OUTPUT          (BYVAL wid AS INTEGER = 0)
PUBLIC DECLARE EXTERN SUB      ON WINDOW CLOSE CALL   (BYVAL wid AS INTEGER, BYVAL p AS SUB (BYVAL INTEGER, BYVAL ANY PTR ), BYVAL ud AS ANY PTR  = NULL)
PUBLIC DECLARE EXTERN SUB      ON WINDOW NEWSIZE CALL (BYVAL wid AS INTEGER, BYVAL p AS SUB (BYVAL INTEGER, BYVAL INTEGER, BYVAL INTEGER, BYVAL ANY PTR ), BYVAL ud AS ANY PTR  = NULL)
PUBLIC DECLARE EXTERN SUB      ON WINDOW REFRESH CALL (BYVAL wid AS INTEGER, BYVAL p AS SUB (BYVAL INTEGER, BYVAL ANY PTR ), BYVAL ud AS ANY PTR  = NULL)
PUBLIC DECLARE EXTERN FUNCTION WINDOW                 (BYVAL n AS INTEGER) AS ULONG
PUBLIC DECLARE EXTERN SUB      MOUSE ON
PUBLIC DECLARE EXTERN SUB      MOUSE OFF
PUBLIC DECLARE EXTERN SUB      ON MOUSE CALL          (BYVAL p AS SUB (BYVAL INTEGER, BYVAL BOOLEAN, BYVAL INTEGER, BYVAL INTEGER, BYVAL ANY PTR ), BYVAL ud AS ANY PTR =NULL)
PUBLIC DECLARE EXTERN FUNCTION MOUSE                  (BYVAL n AS INTEGER) AS INTEGER
PUBLIC DECLARE EXTERN SUB      MOUSE MOTION ON
PUBLIC DECLARE EXTERN SUB      MOUSE MOTION OFF
PUBLIC DECLARE EXTERN SUB      ON MOUSE MOTION CALL   (BYVAL p AS SUB (BYVAL INTEGER, BYVAL BOOLEAN, BYVAL INTEGER, BYVAL INTEGER, BYVAL ANY PTR ), BYVAL ud AS ANY PTR =NULL)
PUBLIC DECLARE EXTERN SUB      SLEEP
PUBLIC DECLARE EXTERN SUB      VWAIT

' drawing

PUBLIC DECLARE EXTERN SUB      LINE                 (_COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=INTEGER_MIN, BYVAL y1 AS INTEGER=INTEGER_MIN, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=INTEGER_MIN, BYVAL y2 AS INTEGER=INTEGER_MIN), _
                                                     BYVAL c AS INTEGER=-1, _LINEBF(BYVAL bf AS INTEGER=0) )
PUBLIC DECLARE EXTERN SUB      PSET                 (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER, BYVAL y AS INTEGER), BYVAL c AS INTEGER=-1)
PUBLIC DECLARE EXTERN FUNCTION POINT                (BYVAL x1 AS INTEGER, BYVAL y AS INTEGER) AS INTEGER
PUBLIC DECLARE EXTERN SUB      CIRCLE               (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y AS INTEGER), BYVAL r AS INTEGER, BYVAL c AS INTEGER=-1, BYVAL start AS INTEGER=0, BYVAL fini AS INTEGER=359, BYVAL ratio AS SINGLE=0.44)

PUBLIC CONST AS INTEGER DRMD_JAM1       = 0
PUBLIC CONST AS INTEGER DRMD_JAM2       = 1
PUBLIC CONST AS INTEGER DRMD_COMPLEMENT = 2
PUBLIC CONST AS INTEGER DRMD_INVERSVID  = 4

PUBLIC DECLARE EXTERN SUB      COLOR                (BYVAL fg AS INTEGER=-1, BYVAL bg AS INTEGER=-1, BYVAL o AS INTEGER=-1, BYVAL drmd AS INTEGER=-1)
PUBLIC DECLARE EXTERN SUB      PAINT                (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER, BYVAL y AS INTEGER), BYVAL pc AS INTEGER=-1, BYVAL bc AS INTEGER=-1)
PUBLIC DECLARE EXTERN SUB      AREA                 (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y AS INTEGER))
PUBLIC DECLARE EXTERN SUB      AREAFILL             (BYVAL mode AS INTEGER=0)
PUBLIC DECLARE EXTERN SUB      AREA OUTLINE         (BYVAL enabled AS BOOLEAN)

PUBLIC DECLARE EXTERN SUB      PATTERN              (BYVAL lineptrn AS UINTEGER = &HFFFF, BYREF areaptrn() AS INTEGER = NULL)
PUBLIC DECLARE EXTERN SUB      PATTERN RESTORE

' ON TIMER support

PUBLIC DECLARE EXTERN SUB      ON TIMER CALL        (BYVAL id AS INTEGER, BYVAL seconds AS SINGLE, BYVAL p AS SUB)
PUBLIC DECLARE EXTERN SUB      TIMER ON             (BYVAL id AS INTEGER)
PUBLIC DECLARE EXTERN SUB      TIMER OFF            (BYVAL id AS INTEGER)

' audio

PUBLIC TYPE WAVE_t
    AS WAVE_t PTR prev, next

    AS ULONG      oneShotHiSamples
    AS ULONG      repeatHiSamples
    AS ULONG      samplesPerHiCycle
    AS ULONG      samplesPerSec
    AS INTEGER    ctOctave
    AS SINGLE     volume

    AS BYTE PTR   data
END TYPE

PUBLIC DECLARE EXTERN FUNCTION WAVE                 (BYREF wd() AS BYTE, _
                                                     BYVAL oneShotHiSamples AS ULONG = 0, _
                                                     BYVAL repeatHiSamples AS ULONG = 32, _
                                                     BYVAL samplesPerHiCycle AS ULONG = 32, _
                                                     BYVAL samplesPerSec AS ULONG = 8192, _
                                                     BYVAL ctOctave AS INTEGER = 1, _
                                                     BYVAL volume AS SINGLE = &H10000) AS WAVE_t PTR
PUBLIC DECLARE EXTERN SUB      WAVE                 (BYVAL channel AS INTEGER, BYVAL w AS WAVE_t PTR)
PUBLIC DECLARE EXTERN SUB      WAVE FREE            (BYVAL w AS WAVE_t PTR)
PUBLIC DECLARE EXTERN SUB      SOUND                (BYVAL freq AS SINGLE=0, BYVAL duration AS SINGLE=0, BYVAL volume AS INTEGER=127, BYVAL channel AS INTEGER=0)
PUBLIC DECLARE EXTERN SUB      SOUND WAIT           (BYVAL channel AS INTEGER=-1)
PUBLIC DECLARE EXTERN SUB      SOUND STOP           (BYVAL channel AS INTEGER=-1)
PUBLIC DECLARE EXTERN SUB      SOUND START          (BYVAL channel AS INTEGER=-1)
