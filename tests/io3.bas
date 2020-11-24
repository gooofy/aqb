'
' test DATA, READ for strings
'

OPTION EXPLICIT

DIM AS INTEGER i, j
DIM AS STRING  s, t

READ i, s
ASSERT i=42
ASSERT s="foo"

READ j, t
ASSERT j=23
ASSERT t="bar baz"

' _debug_puts "i=" : _debug_puts2 i : _debug_puts ", s=" : _debug_puts s : _debug_putnl


DATA 42, foo
DATA 23, "bar baz"

