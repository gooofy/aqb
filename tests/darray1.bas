'
' some basic dynamic array tests
'

OPTION EXPLICIT

DIM a%(100)

a%(1) = 42
'_debug_puts2 a%(1) : _debug_putnl

a%(0) = 23
'_debug_puts2 a%(0) : _debug_putnl

'_debug_puts2 a%(1) : _debug_putnl
'_debug_puts2 a%(0) : _debug_putnl

' PRINT a%(0), a%(1)

ASSERT a%(1) = 42
ASSERT a%(0) = 23

' _debug_puts2 LBOUND(a%) : _debug_putnl
ASSERT LBOUND(a%   ) = 0
ASSERT LBOUND(a%, 0) = 1
ASSERT LBOUND(a%, 2) = 0

' _debug_puts2 UBOUND(a%) : _debug_putnl
ASSERT UBOUND(a%   ) = 100
ASSERT UBOUND(a%, 1) = 100
ASSERT UBOUND(a%, 0) = 1

FOR i AS INTEGER = 0 to 100
  a%(i) = i
NEXT i

FOR i AS INTEGER = 0 to 100
  ASSERT a%(i) = i
  ' PRINT a%(i%)
NEXT i

