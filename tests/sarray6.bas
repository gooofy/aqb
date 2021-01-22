'
' passing static arrays to subprograms
'

OPTION EXPLICIT

TYPE a_t AS INTEGER (STATIC 9)

DIM AS a_t a

FUNCTION sumup (f AS a_t) AS INTEGER

    DIM s AS INTEGER = 0

    FOR i AS INTEGER = LBOUND(f) TO UBOUND(f)
        s = s + f(i)
        ' _debug_puts "sumup i=" : _debug_puts2 i : _debug_puts " -> f(i)=" :_debug_puts2 f(i) : _debug_puts ", s=" : _debug_puts2 s : _debug_putnl
    NEXT i

    RETURN s

END FUNCTION

FOR i AS INTEGER = LBOUND(a) TO UBOUND(a)
    a(i) = i
    '_debug_puts2 i : _debug_puts " -> " :_debug_puts2 i : _debug_putnl
NEXT i

' _debug_puts2 sumup(a) : _debug_putnl

ASSERT sumup (a) = 45

