'
' record type test
'

TYPE t1
    f1 AS INTEGER
    AS LONG f2, f3
END TYPE

DIM v AS t1

v.f1 = 23
v.f2 = 100000
v.f3 = 250000

ASSERT v.f1 = 23
ASSERT v.f2 = 100000
ASSERT v.f3 = 250000

