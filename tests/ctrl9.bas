'
' IF THEN/ELSE namespace test
'

OPTION EXPLICIT

DIM i AS INTEGER = 23

IF i < 23 THEN

    DIM j AS INTEGER = 42

    ' _debug_puts2 j : _debug_putnl

    ASSERT j = 42

ELSE

    DIM j AS INTEGER = 64

    ' _debug_puts2 j : _debug_putnl

    ASSERT j = 64

END IF


