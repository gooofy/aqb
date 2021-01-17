DIM AS ULONG a=18007, b=3, c=3, d=7
CONST AS ULONG ac=18007, bc=3, cc=3, dc=7

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

ASSERT (ac  +  b ) = 18010
ASSERT (a   +  bc) = 18010
ASSERT (ac  +  bc) = 18010
ASSERT (ac  -  b ) = 18004
ASSERT (a   -  bc) = 18004
ASSERT (ac  -  bc) = 18004
ASSERT (ac  *  b ) = 54021
ASSERT (a   *  bc) = 54021
ASSERT (ac  *  bc) = 54021
ASSERT (ac  /  b ) = 6002
ASSERT (a   /  bc) = 6002
ASSERT (ac  /  bc) = 6002
ASSERT (bc  ^  c ) =   27
ASSERT (b   ^  cc) =   27
ASSERT (bc  ^  cc) =   27
ASSERT (ac  \  b ) = 6002
ASSERT (a   \  bc) = 6002
ASSERT (ac  \  bc) = 6002
ASSERT (ac MOD b ) =    1
ASSERT (a  MOD bc) =    1
ASSERT (ac MOD bc) =    1
ASSERT (cc SHL d ) =  384
ASSERT (c  SHL dc) =  384
ASSERT (cc SHL dc) =  384
ASSERT (ac SHR c ) = 2250
ASSERT (a  SHR cc) = 2250
ASSERT (ac SHR cc) = 2250

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
ASSERT (cc XOR d ) =  4
ASSERT ( c XOR dc) =  4
ASSERT (cc XOR dc) =  4
ASSERT ( d XOR d ) =  0
ASSERT ( c EQV d ) = -5
ASSERT (cc EQV d ) = -5
ASSERT ( c EQV dc) = -5
ASSERT (cc EQV dc) = -5
ASSERT ( d EQV d ) = -1
ASSERT ( c IMP d ) = -1
ASSERT (cc IMP d ) = -1
ASSERT ( c IMP dc) = -1
ASSERT (cc IMP dc) = -1
ASSERT ( d IMP d ) = -1
ASSERT ( NOT c   ) = -4
ASSERT ( NOT cc  ) = -4
ASSERT ( NOT d   ) = -8
ASSERT ( c AND d ) =  3
ASSERT (cc AND d ) =  3
ASSERT ( c AND dc) =  3
ASSERT (cc AND dc) =  3
ASSERT ( d AND d ) =  7
ASSERT ( c OR d  ) =  7
ASSERT (cc OR d  ) =  7
ASSERT ( c OR dc ) =  7
ASSERT (cc OR dc ) =  7
ASSERT ( d OR d  ) =  7

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

