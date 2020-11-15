'
' mixing SUB / GOSUB
'

OPTION EXPLICIT

DIM g_i AS INTEGER = 42

SUB foo

    GOSUB l1

END SUB

GOSUB l1

foo

END

l1:
    '_debug_puts "g_i=" : _debug_puts2 g_i : _debug_putnl

    ASSERT g_i = 42

    RETURN

