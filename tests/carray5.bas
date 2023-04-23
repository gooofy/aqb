'
' passing dynamic arrays to subprograms
'

OPTION EXPLICIT

DIM AS INTEGER a(9)

FUNCTION sumup (f() AS INTEGER) AS INTEGER

    DIM s AS INTEGER = 0

    FOR i AS INTEGER = LBOUND(f) TO UBOUND(f)
        s = s + f(i)
    NEXT i

    RETURN s
END FUNCTION

FOR i AS INTEGER = LBOUND(a) TO UBOUND(a)
    a(i) = i
NEXT i

' _debug_puts2 sumup(a) : _debug_putnl

ASSERT sumup (a) = 45

