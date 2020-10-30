'
' dyn array BYREF NULL argument test
'

OPTION EXPLICIT

TYPE a_t AS INTEGER()

DIM a AS a_t

FUNCTION sumup (BYREF f AS a_t = NULL) AS INTEGER

    DIM s AS INTEGER = 0

    FOR i AS INTEGER = LBOUND(f) TO UBOUND(f)
        s = s + f(i)
    NEXT i

    RETURN s
END FUNCTION

REDIM a(9)

FOR i AS INTEGER = LBOUND(a) TO UBOUND(a)
    a(i) = i
NEXT i

' _debug_puts2 sumup(a) : _debug_putnl

ASSERT sumup (a) = 45

ASSERT sumup = 0

