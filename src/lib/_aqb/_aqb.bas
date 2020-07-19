'
' _AQB module interface
'

OPTION EXPLICIT
OPTION PRIVATE

' PRINT statement support:

PUBLIC DECLARE SUB _aio_puts    (s AS STRING  )
PUBLIC DECLARE SUB _aio_puts1   (b AS BYTE    )
PUBLIC DECLARE SUB _aio_puts2   (i AS INTEGER )
PUBLIC DECLARE SUB _aio_puts4   (l AS LONG    )
PUBLIC DECLARE SUB _aio_putu1   (u AS UBYTE   )
PUBLIC DECLARE SUB _aio_putu2   (u AS UINTEGER)
PUBLIC DECLARE SUB _aio_putu4   (u AS ULONG   )
PUBLIC DECLARE SUB _aio_putf    (f AS SINGLE  )
PUBLIC DECLARE SUB _aio_putbool (b AS BOOLEAN )
PUBLIC DECLARE SUB _aio_putnl   ()
PUBLIC DECLARE SUB _aio_puttab  ()

PUBLIC DECLARE SUB      LOCATE  (l AS INTEGER=-1, c AS INTEGER=-1)
PUBLIC DECLARE FUNCTION CSRLIN  () AS INTEGER
PUBLIC DECLARE FUNCTION POS     (dummy AS INTEGER) AS INTEGER

' --------------------------------------------------------------------------------------------------------
' --
' -- AmigaBASIC like screens, windows and graphics
' --
' --------------------------------------------------------------------------------------------------------

' error codes

PUBLIC CONST AS INTEGER ERR_WIN_OPEN      = 64

' window flags

PUBLIC CONST AS INTEGER AW_FLAG_SIZE    =  1
PUBLIC CONST AS INTEGER AW_FLAG_DRAG    =  2
PUBLIC CONST AS INTEGER AW_FLAG_DEPTH   =  4
PUBLIC CONST AS INTEGER AW_FLAG_CLOSE   =  8
PUBLIC CONST AS INTEGER AW_FLAG_REFRESH = 16

' WINDOW id [, [Title] [, [(x1,y1)-(x2,y2)] [, [Flags] [, Screen] ] ]
PUBLIC DECLARE SUB WINDOW (id AS INTEGER, title AS STRING = NULL, _
                           _COORD2(s1 AS BOOLEAN=FALSE, x1 AS INTEGER=-1, y1 AS INTEGER=-1, s2 AS BOOLEAN=FALSE, x2 AS INTEGER=-1, y2 AS INTEGER=-1), _
                           flags AS INTEGER=0, scrid AS INTEGER = 0)

PUBLIC DECLARE SUB WINDOW CLOSE (id AS integer = 0)
PUBLIC DECLARE SUB ON WINDOW CALL (p AS SUB)
PUBLIC DECLARE FUNCTION WINDOW (n AS INTEGER) AS ULONG
PUBLIC DECLARE SUB SLEEP

PUBLIC DECLARE SUB LINE (_COORD2(s1 AS BOOLEAN=FALSE, x1 AS INTEGER=-1, y1 AS INTEGER=-1, s2 AS BOOLEAN=FALSE, x2 AS INTEGER=-1, y2 AS INTEGER=-1), _
                         c AS INTEGER=-1, _LINEBF(bf AS INTEGER=0) )

PUBLIC DECLARE FUNCTION INKEY$ ()

