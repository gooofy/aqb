'
' amiga / ms basic syntax style function calls: multiple syms
'

' SUB      WINDOW id [, Title [ , [ (x1,y1) - (x2,y2) ]  [, [ wintype ] [, scrid] ] ] ] ]
' SUB      WINDOW CLOSE
' SUB      WINDOW OUTPUT id
' FUNCTION WINDOW(x)

' SUB ON BREAK     CALL
' SUB ON COLLISION CALL
' SUB ON ERROR     CALL
' SUB ON MENU      CALL
' SUB ON MOUSE     CALL
' SUB ON TIMER     CALL

' FUNCTION MOUSE()
' SUB      MOUSE ON
' SUB      MOUSE OFF
' SUB      MOUSE STOP


DIM SHARED AS INTEGER sum

SUB MTEST (id AS INTEGER)
    sum = id
END SUB

SUB MTEST FOO (id AS INTEGER)
    sum = 2 * id
END SUB

SUB MTEST FOO BAR (id AS INTEGER)
    sum = 3 * id
END SUB

FUNCTION MTEST (id AS INTEGER) AS INTEGER
    MTEST = 4 * id
END FUNCTION


sum = 0
MTEST 23
ASSERT sum = 23

sum = 0
MTEST FOO 23
ASSERT sum = 46

sum = 0
MTEST FOO BAR  14
ASSERT sum = 42

ASSERT (MTEST(42)=168)


