'
' test STR$() pseudo-function
'

OPTION EXPLICIT

DIM AS BYTE     b   = -42
DIM AS UBYTE    ub  = 200
DIM AS INTEGER  i   = -32000
DIM AS UINTEGER ui  = 65000
DIM AS LONG     l   = -100000
DIM AS ULONG    ul  = 100000
DIM AS BOOLEAN  myb = TRUE

'_debug_puts STR$(b)   : _debug_putnl
'_debug_puts STR$(ub)  : _debug_putnl
'_debug_puts STR$(i)   : _debug_putnl
'_debug_puts STR$(ui)  : _debug_putnl
'_debug_puts STR$(l)   : _debug_putnl
'_debug_puts STR$(ul)  : _debug_putnl
'_debug_puts STR$(myb) : _debug_putnl

ASSERT STR$(b)   = "-42"
ASSERT STR$(ub)  = " 200"
ASSERT STR$(i)   = "-32000"
ASSERT STR$(ui)  = " 65000"
ASSERT STR$(l)   = "-100000"
ASSERT STR$(ul)  = " 100000"
ASSERT STR$(myb) = "TRUE"

