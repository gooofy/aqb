OPTION EXPLICIT

DIM AS INTEGER s=0
FOR i AS INTEGER = 12 TO 1 STEP -1
    s=s+1
NEXT i

' _debug_puts2 s : _debug_putnl

ASSERT s=12
