DIM AS UINTEGER a=18007, b=3

' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp

ASSERT (a  +  b ) = 18010
ASSERT (a  -  b ) = 18004
ASSERT (a  *  b ) = 54021
ASSERT (a  /  b ) = 6002
ASSERT (b  ^  3 ) =   27
ASSERT (a  \  b ) = 6002
ASSERT (a MOD b ) =    1

' same but using constants

ASSERT (a  +  3 ) = 18010
ASSERT (a  -  3 ) = 18004
ASSERT (a  *  3 ) = 54021
ASSERT (a  /  3 ) = 6002
ASSERT (3  ^  3 ) =   27
ASSERT (a  \  3 ) = 6002
ASSERT (a MOD 3 ) =    1

' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp

DIM AS UINTEGER c=3, d=7

ASSERT ( c XOR d ) =  4
ASSERT ( d XOR d ) =  0
ASSERT ( d XOR 1 ) =  6
ASSERT ( c EQV d ) = 65531
ASSERT ( d EQV d ) = 65535
ASSERT ( d EQV 1 ) = -7
ASSERT ( c IMP d ) = 65535
ASSERT ( d IMP d ) = 65535
ASSERT ( d IMP 1 ) = -7
ASSERT ( NOT c   ) = 65532
ASSERT ( NOT d   ) = 65528
ASSERT ( c AND d ) =  3
ASSERT ( d AND d ) =  7
ASSERT ( d AND 1 ) =  1
ASSERT ( c OR d  ) =  7
ASSERT ( d OR d  ) =  7
ASSERT ( d OR 11 ) = 15

' relational operators

ASSERT a =  a
ASSERT NOT (a =  b)
ASSERT a <> b
ASSERT NOT (a <> a)
ASSERT b <  a
ASSERT NOT (a <  a)
ASSERT NOT (a <  b)
ASSERT NOT (b >  a)
ASSERT NOT (a >  a)
ASSERT a >  b
ASSERT b <= a
ASSERT a <= a
ASSERT NOT (a <= b)
ASSERT NOT (b >= a)
ASSERT a >= a
ASSERT a >= b

' conversion tests

DIM AS SINGLE   f  = 42
DIM AS BYTE     b2 = 42
DIM AS UBYTE    ub = 42
DIM AS INTEGER  i  = 42
DIM AS LONG     l  = 42
DIM AS ULONG    ul = 42

b = f
ASSERT b = 42
b = ub
ASSERT b = 42
b = i
ASSERT b = 42
b = b2
ASSERT b = 42
b = l
ASSERT b = 42
b = ul
ASSERT b = 42

