' test local var decl inside FOR ... NEXT

OPTION EXPLICIT

DIM AS INTEGER j=23, s=0

FOR i AS INTEGER = 0 TO 2

    DIM AS INTEGER j=2

    s = s + j

    '_debug_puts "i=" : _debug_puts2 i : _debug_puts ", s=" : _debug_puts2 s : _debug_putnl

NEXT

ASSERT j=23
ASSERT s=6

