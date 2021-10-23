OPTION EXPLICIT

SUB foo (BYVAL i AS INTEGER, BYREF y AS INTEGER)

    y = i

END SUB

DIM AS INTEGER i = 23

foo 42, i

' _debug_puts2 i : _debug_putnl

ASSERT i = 42


