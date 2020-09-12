'
' two-dim static array test
'

OPTION EXPLICIT

' DIM AS INTEGER a(9, 1)
' a(0,0)=1
' 'a(0,1)=1
' 'a(9,0)=2
' 'a(9,1)=3



DIM AS INTEGER a(STATIC 9, 1), b(STATIC 9, 1)

FOR i AS INTEGER = 0 TO 9
    a(i, 0) = i
    a(i, 1) = 42
    b(i, 0) = 23
    b(i, 1) = i
NEXT i

FOR i AS INTEGER = 0 TO 9
    ' _debug_puts2 a(i, 0)
    ' _debug_puts2 a(i, 1)
    ' _debug_puts2 b(i, 0)
    ' _debug_puts2 b(i, 1)
    ASSERT a(i, 0) = i
    ASSERT a(i, 1) = 42
    ASSERT b(i, 0) = 23
    ASSERT b(i, 1) = i
NEXT i

