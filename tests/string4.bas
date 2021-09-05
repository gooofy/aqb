REM string concatenation test

OPTION EXPLICIT

DIM AS STRING a

a = "hubba"

a = a + " foobar"

' _debug_puts a : _debug_putnl

assert a = "hubba foobar"

