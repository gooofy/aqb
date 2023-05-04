'
' dyn array BYREF NULL argument test
'

OPTION EXPLICIT

TYPE a_t AS INTEGER()

DIM a AS a_t

FUNCTION sumup (BYREF f AS a_t = NULL) AS INTEGER

    IF _ISNULL(f) THEN
        '_debug_puts "_ISNULL" : _debug_putnl
        RETURN 0
    END IF
    '_debug_puts "NOT _ISNULL" : _debug_putnl
    '_debug_puts "LBOUND(f)=" : _debug_puts2 LBOUND(f) : _debug_putnl
    '_debug_puts "UBOUND(f)=" : _debug_puts2 UBOUND(f) : _debug_putnl

    DIM s AS INTEGER = 0

    FOR i AS INTEGER = LBOUND(f) TO UBOUND(f)
        s = s + f(i)
    NEXT i

    RETURN s
END FUNCTION

'_debug_puts "before REDIM: " : _debug_putnl
'_debug_puts "LBOUND(a)=" : _debug_puts2 LBOUND(a) : _debug_putnl
'_debug_puts "UBOUND(a)=" : _debug_puts2 UBOUND(a) : _debug_putnl

REDIM a(9)

'_debug_puts "after REDIM: " : _debug_putnl
'_debug_puts "LBOUND(a)=" : _debug_puts2 LBOUND(a) : _debug_putnl
'_debug_puts "UBOUND(a)=" : _debug_puts2 UBOUND(a) : _debug_putnl

FOR i AS INTEGER = LBOUND(a) TO UBOUND(a)
    a(i) = i
NEXT i

'_debug_puts2 sumup(a) : _debug_putnl

ASSERT sumup (a) = 45

'_debug_puts2 sumup : _debug_putnl
ASSERT sumup = 0

