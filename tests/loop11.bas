' test local var decl inside WHILE...WEND

OPTION EXPLICIT

DIM AS INTEGER i=0, j=23, s=0

WHILE i<3

    DIM AS INTEGER j=2

    i = i + 1
    s = s + j

    '_debug_puts "i=" : _debug_puts2 i : _debug_puts ", s=" : _debug_puts2 s : _debug_putnl

WEND

'_debug_puts "i=" : _debug_puts2 i : _debug_putnl
ASSERT i=3
ASSERT j=23
ASSERT s=6

