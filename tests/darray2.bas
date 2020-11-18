'
' whole dynamic array assignment on heap test
'

OPTION EXPLICIT

CONST n AS INTEGER = 99

DIM AS INTEGER a(), b()

REDIM a(n)
REDIM b(n)

'a(1)=2
'b(1)=2

FOR i AS INTEGER = 0 to n
    a(i) = i
    b(i) = i * i
NEXT i

FOR i AS INTEGER = 0 to n
    ' _debug_puts2(a(i)) : _debug_putnl
    ASSERT a(i) = i
NEXT i

a = b

FOR i AS INTEGER = 0 to n
    ASSERT a(i) = i*i
NEXT i

