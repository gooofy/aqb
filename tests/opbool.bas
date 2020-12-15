
DIM a, b AS boolean

a = TRUE
b = FALSE

' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp

ASSERT a XOR b
ASSERT ( a XOR a )      =  FALSE
ASSERT a XOR FALSE
ASSERT ( a EQV b ) = FALSE
ASSERT b EQV b
ASSERT a EQV TRUE
ASSERT b IMP a
ASSERT ( a IMP b ) = FALSE
ASSERT b IMP TRUE
ASSERT ( NOT a ) = FALSE
ASSERT NOT b
ASSERT NOT FALSE
ASSERT a AND a
ASSERT (a AND b ) = FALSE
ASSERT a AND TRUE
ASSERT (b AND FALSE) = FALSE
ASSERT a OR b
ASSERT NOT (b OR b)
ASSERT NOT (b OR FALSE)
ASSERT b OR TRUE
ASSERT a OR TRUE

' relational operators

ASSERT a <> B
ASSERT a = a
ASSERT NOT (a = b)
ASSERT NOT (a <> a)

' conversion tests

DIM i AS integer
DIM f AS single
DIM l AS long

i = 42
f = 1.0
l = 23

b = i
ASSERT b
b = f
ASSERT b
b = l
ASSERT b

i = 0
f = 0.0
l = 0

b = i
ASSERT NOT b
b = f
ASSERT NOT b
b = l
ASSERT NOT b

