'
' test DATA, READ
'

OPTION EXPLICIT

DIM AS INTEGER i, j, k, l, m, n

READ i, j, k, l, m, n

' _debug_puts "i=" : _debug_puts2 i : _debug_puts ", j=" : _debug_puts2 j : _debug_putnl
' _debug_puts "k=" : _debug_puts2 k : _debug_puts ", l=" : _debug_puts2 l : _debug_putnl
' _debug_puts "m=" : _debug_puts2 m : _debug_puts ", n=" : _debug_puts2 n : _debug_putnl

ASSERT i=1
ASSERT j=2
ASSERT k=3
ASSERT l=4
ASSERT m=5
ASSERT n=6

DATA 1,2,3,4,5,6,7,8

