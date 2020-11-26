'
' test RESTORE
'

OPTION EXPLICIT

DIM AS INTEGER i, j, k

READ i, j, k

' _debug_puts "i=" : _debug_puts2 i : _debug_puts ", j=" : _debug_puts2 j : _debug_putnl

ASSERT i=1
ASSERT j=2
ASSERT k=3

RESTORE
READ i
'  _debug_puts "i=" : _debug_puts2 i : _debug_putnl
ASSERT i=1

RESTORE dataset2

READ i, j, k

' _debug_puts "i=" : _debug_puts2 i : _debug_puts ", j=" : _debug_puts2 j : _debug_putnl

ASSERT i=10
ASSERT j=11
ASSERT k=12

dataset1:

DATA 1,2,3

dataset2:

DATA 10,11,12

