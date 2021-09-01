OPTION EXPLICIT

DIM s AS INTEGER = 0

FOR w AS SINGLE = 0 TO 3.14 STEP 0.1
    s = s + 1
NEXT W

' _debug_puts2 s : _debug_putnl

ASSERT s = 32

