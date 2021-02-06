' register pressure test for linscan

OPTION EXPLICIT

SUB mysub (BYVAL b1 AS BOOLEAN, BYVAL x1 AS INTEGER, BYVAL y1 AS INTEGER, BYVAL b2 AS BOOLEAN, BYVAL x2 AS INTEGER, BYVAL y2 AS INTEGER, BYVAL a AS INTEGER, BYVAL b AS INTEGER)

    ' _debug_puts "b1="   : _debug_puts2 b1
    ' _debug_puts ", x1=" : _debug_puts2 x1
    ' _debug_puts ", y1=" : _debug_puts2 y1
    ' _debug_puts ", b2=" : _debug_puts2 b2
    ' _debug_puts ", x2=" : _debug_puts2 x2
    ' _debug_puts ", y2=" : _debug_puts2 y2
    ' _debug_puts ", a="  : _debug_puts2 a
    ' _debug_puts ", b="  : _debug_puts2 b
    ' _debug_putnl

    ASSERT b1 = -1
    ASSERT x1 = 1
    ASSERT y1 = 2
    ASSERT b2 = 0
    ASSERT x2 = 3
    ASSERT y2 = 4
    ASSERT a  = 23
    ASSERT b  = 42

END SUB

mysub TRUE, 1, 2, FALSE, 3, 4, 23, 42


