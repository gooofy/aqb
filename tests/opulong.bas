DIM AS ULONG a=18007, b=3, c=3, d=7

' arithemtic operators
' CG_plus,  CG_minus,  CG_mul, CG_div, CG_neg
' CG_power, CG_intDiv, CG_mod, CG_shl, CG_shr

ASSERT (a  +  b ) = 18010
ASSERT (a  -  b ) = 18004
ASSERT (a  *  b ) = 54021
ASSERT (a  /  b ) = 6002
ASSERT (b  ^  c ) =   27
ASSERT (a  \  b ) = 6002
ASSERT (a MOD b ) =    1
ASSERT (c SHL d ) =  384
ASSERT (a SHR c ) = 2250

' same but using constants

ASSERT (    a  +  3 ) = 18010
ASSERT (    3  +  a ) = 18010
ASSERT (    a  -  3 ) = 18004
ASSERT (18007  -  b ) = 18004
ASSERT (    a  *  3 ) = 54021
ASSERT (    3  *  a ) = 54021
ASSERT (    a  /  3 ) =  6002
ASSERT (18006  /  b ) =  6002
ASSERT (    a  \  3 ) =  6002
ASSERT (18007  \  b ) =  6002
ASSERT (    3  ^  b ) =    27
ASSERT (    b  ^  3 ) =    27
ASSERT (    a MOD 3 ) =     1
ASSERT (18007 MOD b ) =     1
ASSERT (    1 SHL d ) =   128
' 00000011 = 3
'    11000 =24
ASSERT (    c SHL 3 ) =    24
ASSERT (  128 SHR c ) =    16
' 0100 0110 0101 0111 = 18007
' 0000 1000 1100 1010 = 2250
ASSERT (    a SHR 3 ) =  2250

' test ADD optimizations

ASSERT b + 0  =    3   : REM identity
ASSERT b + 1  =    4   : REM ADDQ
ASSERT b + 8  =   11   : REM ADDQ

ASSERT 0 + b  =    3   : REM identity
ASSERT 1 + b  =    4   : REM ADDQ
ASSERT 8 + b  =   11   : REM ADDQ

' test SUB optimizations

ASSERT a - 0  = 18007  : REM identity
ASSERT a - 1  = 18006  : REM SUBQ
ASSERT a - 8  = 17999  : REM SUBQ

' test MUL optimizations

ASSERT b*0   = 0
ASSERT b*1   = 3
ASSERT b*2   = 6
ASSERT b*4   = 12
ASSERT b*8   = 24
ASSERT b*16  = 48
ASSERT b*32  = 96
ASSERT b*64  = 192
ASSERT b*128 = 384
ASSERT b*256 = 768

ASSERT 0  *b = 0
ASSERT 1  *b = 3
ASSERT 2  *b = 6
ASSERT 4  *b = 12
ASSERT 8  *b = 24
ASSERT 16 *b = 48
ASSERT 32 *b = 96
ASSERT 64 *b = 192
ASSERT 128*b = 384
ASSERT 256*b = 768


' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp


ASSERT ( c XOR d ) =  4
ASSERT ( d XOR d ) =  0
ASSERT ( d XOR 1 ) =  6
ASSERT ( 3 XOR d ) =  4
ASSERT ( c EQV d ) = -5
ASSERT ( d EQV d ) = -1
ASSERT ( d EQV 1 ) = -7
ASSERT ( 3 EQV d ) = -5
ASSERT ( c IMP d ) = -1
ASSERT ( d IMP d ) = -1
ASSERT ( d IMP 1 ) = -7
ASSERT ( 3 IMP d ) = -1
ASSERT ( NOT c   ) = -4
ASSERT ( NOT d   ) = -8
ASSERT ( c AND d ) =  3
ASSERT ( d AND d ) =  7
ASSERT ( d AND 1 ) =  1
ASSERT ( 3 AND d ) =  3
ASSERT ( c OR d  ) =  7
ASSERT ( d OR d  ) =  7
ASSERT ( d OR 11 ) = 15
ASSERT ( 3 OR d  ) =  7

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
DIM AS UINTEGER ui  = 42
DIM AS LONG     l  = 42

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
b = ui
ASSERT b = 42

