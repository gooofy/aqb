'
' _AQB module interface
'

OPTION EXPLICIT
OPTION PRIVATE

' PRINT statement support:

PUBLIC DECLARE SUB _aio_puts    (BYVAL fno AS UINTEGER, BYVAL s AS STRING  )
PUBLIC DECLARE SUB _aio_puts1   (BYVAL fno AS UINTEGER, BYVAL b AS BYTE    )
PUBLIC DECLARE SUB _aio_puts2   (BYVAL fno AS UINTEGER, BYVAL i AS INTEGER )
PUBLIC DECLARE SUB _aio_puts4   (BYVAL fno AS UINTEGER, BYVAL l AS LONG    )
PUBLIC DECLARE SUB _aio_putu1   (BYVAL fno AS UINTEGER, BYVAL u AS UBYTE   )
PUBLIC DECLARE SUB _aio_putu2   (BYVAL fno AS UINTEGER, BYVAL u AS UINTEGER)
PUBLIC DECLARE SUB _aio_putu4   (BYVAL fno AS UINTEGER, BYVAL u AS ULONG   )
PUBLIC DECLARE SUB _aio_putf    (BYVAL fno AS UINTEGER, BYVAL f AS SINGLE  )
PUBLIC DECLARE SUB _aio_putbool (BYVAL fno AS UINTEGER, BYVAL b AS BOOLEAN )
PUBLIC DECLARE SUB _aio_putnl   (BYVAL fno AS UINTEGER)
PUBLIC DECLARE SUB _aio_puttab  (BYVAL fno AS UINTEGER)

' [ LINE ] INPUT support:

PUBLIC DECLARE SUB _aio_line_input             (BYVAL prompt AS STRING, BYREF s AS STRING, BYVAL do_nl AS BOOLEAN)
PUBLIC DECLARE SUB _aio_console_input          (BYVAL qm AS BOOLEAN, BYVAL prompt AS STRING, BYVAL do_nl AS BOOLEAN)
PUBLIC DECLARE SUB _aio_inputs1                (BYREF v AS BYTE    )
PUBLIC DECLARE SUB _aio_inputu1                (BYREF v AS UBYTE   )
PUBLIC DECLARE SUB _aio_inputs2                (BYREF v AS INTEGER )
PUBLIC DECLARE SUB _aio_inputu2                (BYREF v AS UINTEGER)
PUBLIC DECLARE SUB _aio_inputs4                (BYREF v AS LONG    )
PUBLIC DECLARE SUB _aio_inputu4                (BYREF v AS ULONG   )
PUBLIC DECLARE SUB _aio_inputf                 (BYREF v AS SINGLE  )
PUBLIC DECLARE SUB _aio_inputs                 (BYREF v AS STRING  )
PUBLIC DECLARE SUB _aio_set_dos_cursor_visible (BYVAL visible AS BOOLEAN)


PUBLIC DECLARE SUB      LOCATE  (BYVAL l AS INTEGER=-1, BYVAL c AS INTEGER=-1)
PUBLIC DECLARE FUNCTION CSRLIN  () AS INTEGER
PUBLIC DECLARE FUNCTION POS     (BYVAL dummy AS INTEGER) AS INTEGER

' file i/o

PUBLIC CONST AS UINTEGER FILE_MODE_RANDOM      = 0
PUBLIC CONST AS UINTEGER FILE_MODE_INPUT       = 1
PUBLIC CONST AS UINTEGER FILE_MODE_OUTPUT      = 2
PUBLIC CONST AS UINTEGER FILE_MODE_APPEND      = 3
PUBLIC CONST AS UINTEGER FILE_MODE_BINARY      = 4

PUBLIC CONST AS UINTEGER FILE_ACCESS_READ      = 0
PUBLIC CONST AS UINTEGER FILE_ACCESS_WRITE     = 1
PUBLIC CONST AS UINTEGER FILE_ACCESS_READWRITE = 2

PUBLIC DECLARE SUB _aio_open  (BYVAL fname AS STRING, BYVAL mode AS UINTEGER, BYVAL access AS UINTEGER, BYVAL fno AS UINTEGER, BYVAL recordlen AS UINTEGER)
PUBLIC DECLARE SUB _aio_close (BYVAL fno AS UINTEGER)

' --------------------------------------------------------------------------------------------------------
' --
' -- bitmaps, screens, windows and graphics
' --
' --------------------------------------------------------------------------------------------------------

' error codes

PUBLIC CONST AS INTEGER ERR_WIN_OPEN          = 101
PUBLIC CONST AS INTEGER ERR_SCREEN_OPEN       = 102
PUBLIC CONST AS INTEGER ERR_PALETTE           = 103
PUBLIC CONST AS INTEGER ERR_COLOR             = 104
PUBLIC CONST AS INTEGER ERR_AREA              = 105
PUBLIC CONST AS INTEGER ERR_PATTERN           = 106
PUBLIC CONST AS INTEGER ERR_WIN_CLOSE         = 107
PUBLIC CONST AS INTEGER ERR_WIN_OUTPUT        = 108
PUBLIC CONST AS INTEGER ERR_SCREEN_CLOSE      = 109
PUBLIC CONST AS INTEGER ERR_PAINT             = 110
PUBLIC CONST AS INTEGER ERR_LINE              = 111
PUBLIC CONST AS INTEGER ERR_PSET              = 112
PUBLIC CONST AS INTEGER ERR_INPUT_OUT_OF_DATA = 113
PUBLIC CONST AS INTEGER AE_ON_TIMER_CALL      = 114
PUBLIC CONST AS INTEGER AE_TIMER_ON           = 115
PUBLIC CONST AS INTEGER AE_TIMER_OFF          = 116
PUBLIC CONST AS INTEGER AE_OPEN               = 117
PUBLIC CONST AS INTEGER AE_OUTPUT             = 118
PUBLIC CONST AS INTEGER AE_CLOSE              = 119
PUBLIC CONST AS INTEGER AE_MOUSE              = 120
PUBLIC CONST AS INTEGER AE_BLIT               = 121
PUBLIC CONST AS INTEGER AE_RASTPORT           = 122

' colors and palettes

PUBLIC TYPE COLOR_t
    AS UBYTE    r, g, b
END TYPE

PUBLIC TYPE PALETTE_t
    AS INTEGER  numEntries
    AS COLOR_t  colors(256)
END TYPE

PUBLIC DECLARE SUB      PALETTE         (BYVAL cid AS INTEGER, BYVAL red AS SINGLE, BYVAL green AS SINGLE, BYVAL blue AS SINGLE)
PUBLIC DECLARE SUB      PALETTE LOAD    (BYVAL p AS PALETTE_t PTR)

PUBLIC DECLARE FUNCTION INKEY$          ()

' BitMaps

PUBLIC TYPE BITMAP_t
    AS BITMAP_t PTR     prev, next
    AS INTEGER          width, height
    AS BOOLEAN          cont
    ' FIXME struct BitMap   bm
    ' FIXME struct RastPort rp
END TYPE

PUBLIC DECLARE FUNCTION BITMAP          (BYVAL width AS INTEGER, BYVAL height AS INTEGER, BYVAL depth AS INTEGER, BYVAL cont AS BOOLEAN=FALSE) AS BITMAP_t PTR
PUBLIC DECLARE SUB      BITMAP FREE     (BYVAL bm AS BITMAP_t PTR)
PUBLIC DECLARE SUB      GET             (_COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=-1, BYVAL y1 AS INTEGER=-1, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1), BYVAL blit AS VOID PTR)
PUBLIC DECLARE SUB      PUT             (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER=-1, BYVAL y AS INTEGER=-1), BYVAL blit AS VOID PTR, BYVAL minterm AS UBYTE=&HC0, _
                                         _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=-1, BYVAL y1 AS INTEGER=-1, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1))

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

PUBLIC DECLARE SUB SCREEN (BYVAL id AS INTEGER, BYVAL width AS INTEGER, BYVAL height AS INTEGER, BYVAL depth AS INTEGER, BYVAL mode AS UINTEGER, BYVAL title AS STRING=NULL, BYVAL bm AS BITMAP_t PTR=NULL)
PUBLIC DECLARE SUB SCREEN CLOSE (BYVAL id AS INTEGER)

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
PUBLIC DECLARE SUB      WINDOW (BYVAL id AS INTEGER, BYVAL title AS STRING = NULL, _
                                _COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=-1, BYVAL y1 AS INTEGER=-1, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1), _
                                BYVAL flags AS ULONG=AW_FLAG_SIZEGADGET OR AW_FLAG_DRAGBAR OR AW_FLAG_DEPTHGADGET OR AW_FLAG_CLOSEGADGET OR AW_FLAG_SMART_REFRESH OR AW_FLAG_GIMMEZEROZERO OR AW_FLAG_ACTIVATE, _
                                BYVAL scrid AS INTEGER = 0)

PUBLIC DECLARE SUB      WINDOW CLOSE         (BYVAL id AS INTEGER = 0)
PUBLIC DECLARE SUB      WINDOW OUTPUT        (BYVAL id AS INTEGER = 0)
PUBLIC DECLARE SUB      ON WINDOW CALL       (BYVAL p AS SUB)
PUBLIC DECLARE FUNCTION WINDOW               (BYVAL n AS INTEGER) AS ULONG
PUBLIC DECLARE SUB      MOUSE ON
PUBLIC DECLARE SUB      MOUSE OFF
PUBLIC DECLARE SUB      ON MOUSE CALL        (BYVAL p AS SUB)
PUBLIC DECLARE FUNCTION MOUSE                (BYVAL n AS INTEGER) AS INTEGER
PUBLIC DECLARE SUB      MOUSE MOTION ON
PUBLIC DECLARE SUB      MOUSE MOTION OFF
PUBLIC DECLARE SUB      ON MOUSE MOTION CALL (BYVAL p AS SUB)
PUBLIC DECLARE SUB      SLEEP
PUBLIC DECLARE SUB      SLEEP FOR            (BYVAL s AS SINGLE)
PUBLIC DECLARE SUB      VWAIT

' drawing

PUBLIC DECLARE SUB      CLS
PUBLIC DECLARE SUB      LINE                 (_COORD2(BYVAL s1 AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER=-1, BYVAL y1 AS INTEGER=-1, BYVAL s2 AS BOOLEAN=FALSE, BYVAL x2 AS INTEGER=-1, BYVAL y2 AS INTEGER=-1), _
                                              BYVAL c AS INTEGER=-1, _LINEBF(BYVAL bf AS INTEGER=0) )
PUBLIC DECLARE SUB      PSET                 (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y AS INTEGER), BYVAL c AS INTEGER=-1)
PUBLIC DECLARE SUB      COLOR                (BYVAL fg AS INTEGER=-1, BYVAL bg AS INTEGER=-1, BYVAL o AS INTEGER=-1)
PUBLIC DECLARE SUB      PAINT                (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x AS INTEGER, BYVAL y AS INTEGER), BYVAL pc AS INTEGER=-1, BYVAL bc AS INTEGER=-1)
PUBLIC DECLARE SUB      AREA                 (_COORD(BYVAL s AS BOOLEAN=FALSE, BYVAL x1 AS INTEGER, BYVAL y AS INTEGER))
PUBLIC DECLARE SUB      AREAFILL             (BYVAL mode AS INTEGER=0)
PUBLIC DECLARE SUB      AREA OUTLINE         (BYVAL enabled AS BOOLEAN)

PUBLIC DECLARE SUB      PATTERN              (BYVAL lineptrn AS UINTEGER = &HFFFF, BYREF areaptrn() AS INTEGER = NULL)
PUBLIC DECLARE SUB      PATTERN RESTORE

' ON TIMER support

PUBLIC DECLARE SUB ON TIMER CALL        (BYVAL id AS INTEGER, BYVAL seconds AS SINGLE, BYVAL p AS SUB)
PUBLIC DECLARE SUB TIMER ON             (BYVAL id AS INTEGER)
PUBLIC DECLARE SUB TIMER OFF            (BYVAL id AS INTEGER)

