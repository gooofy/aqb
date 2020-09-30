'
' whole dynamic array assignment on heap test
'

OPTION EXPLICIT

DIM AS INTEGER a(), b()

REDIM a(99)
REDIM b(99)

a(1)=2

FOR i AS INTEGER = 0 to 99
  a(i) = i
  b(i) = i * i
NEXT i

FOR i AS INTEGER = 0 to 99
  ASSERT a(i) = i
NEXT i

a = b

FOR i AS INTEGER = 0 to 99
  ASSERT a(i) = i*i
NEXT i

