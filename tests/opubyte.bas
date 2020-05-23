DIM AS UBYTE a=23, b=2

' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp

ASSERT (a  +  b ) =   25
ASSERT (a  -  b ) =   21
ASSERT (a  *  b ) =   46
ASSERT (a  /  b ) =   11
ASSERT (b  ^  3 ) =    8
ASSERT (a  \  b ) =   11
ASSERT (a MOD b ) =    1

' same but using constants

ASSERT (a  +  2 ) =   25
ASSERT (a  -  2 ) =   21
ASSERT (a  *  2 ) =   46
ASSERT (a  /  2 ) =   11
ASSERT (2  ^  3 ) =    8
ASSERT (a  \  2 ) =   11
ASSERT (a MOD 2 ) =    1


' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp

DIM AS UBYTE c=3, d=7

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

ASSERT ( b  SHL c ) =  16
ASSERT ( a  SHR c ) =   2

ASSERT ( 1  SHL c ) =   8
ASSERT (128 SHR c ) =  16

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
DIM AS INTEGER  i  = 42
DIM AS UINTEGER ui = 42
DIM AS LONG     l  = 42
DIM AS ULONG    ul = 42

b = f
ASSERT b = 42
b = b2
ASSERT b = 42
b = i
ASSERT b = 42
b = ui
ASSERT b = 42
b = l
ASSERT b = 42
b = ul
ASSERT b = 42

