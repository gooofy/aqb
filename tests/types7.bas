'
' UDT pointer test
'

TYPE udt
    i%
    j AS INTEGER PTR
    l&
    AS LONG PTR p1, p2, p3
END TYPE

DIM u AS udt

u.i% = 42
u.j = @u.i%
u.l& = 100000
u.p1 = @u.l&
u.p2 = @u.l&
u.p3 = @u.l&

ASSERT u.i% = 42
ASSERT *u.j = 42

ASSERT u.l& = 100000
ASSERT *u.p1 = 100000
ASSERT *u.p2 = 100000
ASSERT *u.p3 = 100000

u.i% = 23
ASSERT u.i% = 23
ASSERT *u.j = 23

u.l& = 200000
ASSERT u.l& = 200000
ASSERT *u.p1 = 200000
ASSERT *u.p2 = 200000
ASSERT *u.p3 = 200000



