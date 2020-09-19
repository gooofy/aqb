'
' pointer type test
'

DIM v AS INTEGER PTR

DIM i AS INTEGER
DIM j AS INTEGER

v = VARPTR(i)

i = 42

ASSERT i = 42
ASSERT *v = 42

*v = 23

ASSERT i = 23
ASSERT *v = 23

v = @j
j = 42

ASSERT i = 23
ASSERT j = 42
ASSERT *v = 42

