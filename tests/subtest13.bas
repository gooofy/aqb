'
' test proc pointers: function with two args
'

OPTION EXPLICIT

DIM SHARED g_p AS FUNCTION (INTEGER, INTEGER) AS INTEGER

FUNCTION q(x AS INTEGER, y AS INTEGER) AS INTEGER
    q = x*y
END FUNCTION

FUNCTION p(x AS INTEGER, y AS INTEGER) AS INTEGER
    p = x+y
END FUNCTION

SUB setfp (BYVAL fp AS FUNCTION (INTEGER, INTEGER) AS INTEGER)
    g_p = fp
END SUB

'_debug_puts "1" : _debug_putnl

setfp q
'_debug_puts "2" : _debug_putnl
ASSERT g_p (6, 7) = 42

' _debug_puts "3" : _debug_putnl
setfp p
' _debug_puts "4" : _debug_putnl
ASSERT g_p (16, 7) = 23

