'
' dyn array REDIM PRESERVE test
'

OPTION EXPLICIT

TYPE a_t AS INTEGER()

DIM a AS a_t

FUNCTION sumup (BYREF f AS a_t) AS INTEGER

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

ASSERT sumup (a) = 45

REDIM PRESERVE a(19)

FOR i AS INTEGER = 10 TO 19
    a(i) = i
NEXT i

ASSERT sumup (a) = 190

