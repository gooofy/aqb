OPTION EXPLICIT

SUB bar (BYREF x AS INTEGER)

    x = 42

END SUB

SUB foo (BYREF y AS INTEGER)

    bar y

END SUB

DIM AS INTEGER i = 23

foo i

' _debug_puts2 i : _debug_putnl

ASSERT i = 42


