'
' two-dim dynamic array test
'

OPTION EXPLICIT

DIM AS INTEGER a(9, 1), b(9, 1)

FOR i AS INTEGER = 0 TO 9
   ' _debug_puts "fill: i=" :_debug_puts2 i : _debug_putnl
    a(i, 0) = i
    a(i, 1) = 42
    b(i, 0) = 23
    b(i, 1) = i
NEXT i

FOR i AS INTEGER = 0 TO 9
    ' _debug_puts "read: i=" :_debug_puts2 i : _debug_putnl
    ASSERT a(i, 0) = i
    ASSERT a(i, 1) = 42
    ASSERT b(i, 0) = 23
    ASSERT b(i, 1) = i
NEXT i

