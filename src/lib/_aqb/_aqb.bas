'
' _AQB module interface
'

OPTION EXPLICIT
OPTION PRIVATE

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


PUBLIC DECLARE SUB LINE (_COORD2(s1 AS BOOLEAN=FALSE, x1 AS INTEGER=-1, y1 AS INTEGER=-1, s2 AS BOOLEAN=FALSE, x2 AS INTEGER=-1, y2 AS INTEGER=-1), _
                         c AS INTEGER=-1, _LINEBF(bf AS INTEGER=0) )


PUBLIC DECLARE SUB SLEEP

