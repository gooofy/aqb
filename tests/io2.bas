'
' test DATA, READ for strings
'

OPTION EXPLICIT

DIM AS INTEGER i, j
DIM AS STRING  s, t

READ i, s

' _debug_puts "i=" : _debug_puts2 i : _debug_puts ", s=" : _debug_puts s : _debug_putnl

ASSERT i=42
ASSERT s="foo"

DATA 42, foo
DATA 23, "bar baz"

