a = 3.14
b = 3.14e+2
c = 3.14e-2
d = 3.0

print "a ="; a
print "b ="; b
print "c ="; c


' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp

ASSERT (a  +  b) =   317.14
ASSERT (a  +  c) =   3.1714
ASSERT (a  -  b) =  -310.86
ASSERT (a  *  b) =   985.96
ASSERT (b  /  a) =   100.00
ASSERT INT((b^c)*1000.0) =  1197
ASSERT (b  \  a) =   100.00
ASSERT (b MOD a) =        0
ASSERT INT((a MOD d) * 100) = 14

' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp

c = 3! : d = 7!

ASSERT ( c XOR d ) =  4
ASSERT ( d XOR d ) =  0
ASSERT ( d XOR 1 ) =  6

ASSERT ( c EQV d ) = -5
ASSERT ( d EQV d ) = -1
ASSERT ( d EQV 1 ) = -7

ASSERT ( c IMP d ) = -1
ASSERT ( d IMP d ) = -1
ASSERT ( d IMP 1 ) = -7

ASSERT ( NOT c   ) = -4
ASSERT ( NOT d   ) = -8

ASSERT ( c AND d ) =  3
ASSERT ( d AND d ) =  7
ASSERT ( d AND 1 ) =  1

ASSERT ( c OR d  ) =  7
ASSERT ( d OR d  ) =  7
ASSERT ( d OR 11 ) = 15

' relational operators

ASSERT (a = a)
ASSERT NOT( a =  b)
ASSERT  a <> b
ASSERT NOT( a <> a)
ASSERT  a <  b
ASSERT NOT( a <  a)
ASSERT NOT( b <  a)
ASSERT NOT( a >  b)
ASSERT NOT( a >  a)
ASSERT  b >  a
ASSERT  a <= b
ASSERT  a <= a
ASSERT NOT( b <= a)
ASSERT NOT( a >= b)
ASSERT  a >= a
ASSERT  b >= a

' conversion tests

iT1% = 42
lT1& = 100000

f = iT1%
ASSERT f=     42
f = iT1% > 23
ASSERT f=     -1
f = lT1&
ASSERT f= 100000
