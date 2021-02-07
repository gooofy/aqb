' test for loop step vs boundaries

OPTION EXPLICIT

DIM AS INTEGER arr_sz_x=101

CONST AS INTEGER prec_x=5

DIM AS INTEGER cnt=0

FOR gx AS INTEGER = 0 TO arr_sz_x - prec_x STEP prec_x
    '_debug_puts "arr_sz_x=" : _debug_puts2 arr_sz_x : _debug_puts ", arr_sz_x - prec_x=" : _debug_puts2 arr_sz_x - prec_x : _debug_puts ", gx=" : _debug_puts2 gx : _debug_putnl
    cnt = cnt + 1
NEXT gx

'_debug_puts2 cnt : _debug_putnl

ASSERT cnt=20
