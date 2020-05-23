a = 3.14
b = 3.14e+2
c = 3.14e-2
d = 3.0

f = 0.5

' PRINT "a ="; a
' PRINT "b ="; b
' PRINT "c ="; c


' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp

ASSERT (a  +  b) =   317.14
ASSERT (a  +  c) =   3.1714
ASSERT (a  -  b) =  -310.86
ASSERT (a  *  b) =   985.96
ASSERT (b  /  a) =   100.00
ASSERT INT((b^c)*1000.0) =  1198
ASSERT (b  \  a) =   100.00
ASSERT (b MOD a) =        2

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

ASSERT INT(c SHL d) = 384
ASSERT INT(b SHR c) = 39

ASSERT INT(1! SHL 7!) = 128
ASSERT INT(128! SHR 6!) = 2

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

' rounding tests (FIXME: some of these are not equivalent to QuickBasic!)

' PRINT INT(2.5), CINT(2.5), FIX(2.5)
' PRINT INT(-2.5), CINT(-2.5), FIX(-2.5)

ASSERT(INT ( 2.5) = 3)
ASSERT(CINT( 2.5) = 3)
ASSERT(FIX ( 2.5) = 2)

ASSERT(INT (-2.5) =-3)
ASSERT(CINT(-2.5) =-3)
ASSERT(FIX (-2.5) =-2)

' PRINT INT(3.5), CINT(3.5), FIX(3.5)
' PRINT INT(-3.5), CINT(-3.5), FIX(-3.5)

ASSERT(INT ( 3.5) = 4)
ASSERT(CINT( 3.5) = 4)
ASSERT(FIX ( 3.5) = 3)

ASSERT(INT (-3.5) =-4)
ASSERT(CINT(-3.5) =-4)
ASSERT(FIX (-3.5) =-3)

