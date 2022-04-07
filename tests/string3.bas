'
' test VAL*() functions
'

OPTION EXPLICIT

'DIM AS BYTE     b   = -42
'DIM AS UBYTE    ub  = 200
'DIM AS INTEGER  i   = -32000
'DIM AS UINTEGER ui  = 65000
'DIM AS LONG     l   = -100000
'DIM AS ULONG    ul  = 100000
'DIM AS BOOLEAN  myb = TRUE


ASSERT VAL("10")      = 10
ASSERT VAL("-271.3")  = -271.3
ASSERT VAL("&h10")    = 16
ASSERT VAL("&o10")    = 8
ASSERT VAL("&10")     = 8
ASSERT VAL("&b10")    = 2
ASSERT VAL("123a45")  = 123
ASSERT VAL("  10")    = 10
'TRACE CINT(VAL(".234")*1000), 234
ASSERT CINT(VAL(".234")*1000)    = 234
ASSERT VAL("a321")    = 0
ASSERT VAL("+123")    = 123

' _debug_putf VAL("2.13E+2") : _debug_putnl
' _debug_puts2 FIX(VAL("2.13E+2")) : _debug_putnl
ASSERT FIX(VAL("2.13E+2")) = 212



'
' VALINT
'

'_debug_puts2 VALINT(".12345") : _debug_putnl
ASSERT VALINT(".12345") = 0

'_debug_puts2 VALINT("&h1ABC") : _debug_putnl
ASSERT VALINT("&h1ABC") = 6844

'_debug_puts2 VALINT("   -42") : _debug_putnl
ASSERT VALINT("   -42") = -42

'_debug_puts2 VALINT("   +23") : _debug_putnl
ASSERT VALINT("   +23") = 23

'_debug_puts2 VALINT("12.987") : _debug_putnl
ASSERT VALINT("12.987") = 12

'_debug_puts2 VALINT( "133e7") : _debug_putnl
ASSERT VALINT( "133e7") = 133 : REM no support for scientific notation

'
' VALUINT
'

'_debug_puts2 VALUINT(".12345") : _debug_putnl
ASSERT VALUINT(".12345") = 0

'_debug_puts2 VALUINT("&h1ABC") : _debug_putnl
ASSERT VALUINT("&h1ABC") = 6844

'_debug_putu4 VALUINT("   -42") : _debug_putnl
ASSERT VALUINT("   -42") = 65494

'_debug_puts2 VALUINT("12.987") : _debug_putnl
ASSERT VALUINT("12.987") = 12

'_debug_puts2 VALUINT( "133e7") : _debug_putnl
ASSERT VALUINT( "133e7") = 133 : REM no support for scientific notation

'
' VALLNG
'

'_debug_puts2 VALLNG(".12345") : _debug_putnl
ASSERT VALLNG(".12345") = 0

'_debug_puts2 VALLNG("&h1ABC") : _debug_putnl
ASSERT VALLNG("&h1ABC") = 6844

'_debug_putu4 VALLNG("   -42") : _debug_putnl
ASSERT VALLNG("   -42") = -42

'_debug_puts2 VALLNG("12.987") : _debug_putnl
ASSERT VALLNG("12.987") = 12

'_debug_puts2 VALLNG( "133e7") : _debug_putnl
ASSERT VALLNG( "133e7") = 133 : REM no support for scientific notation

'_debug_putu4 VALLNG( "123456") : _debug_putnl
ASSERT VALLNG( "123456") = 123456

'
' VALULNG
'

'_debug_puts2 VALULNG(".12345") : _debug_putnl
ASSERT VALULNG(".12345") = 0

'_debug_puts2 VALULNG("&h1ABC") : _debug_putnl
ASSERT VALULNG("&h1ABC") = 6844

'_debug_putu4 VALULNG("   -42") : _debug_putnl
ASSERT VALULNG("   -42") = 4294967254

'_debug_puts2 VALULNG("12.987") : _debug_putnl
ASSERT VALULNG("12.987") = 12

'_debug_puts2 VALULNG( "133e7") : _debug_putnl
ASSERT VALULNG( "133e7") = 133 : REM no support for scientific notation

'_debug_putu4 VALULNG( "123456") : _debug_putnl
ASSERT VALULNG( "123456") = 123456

