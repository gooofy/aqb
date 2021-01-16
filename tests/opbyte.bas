DIM AS BYTE a=23, b=2
DIM AS BYTE c=3, d=7
CONST AS BYTE ac=23, bc=2, cc=3, dc=7, ec=64

' arithemtic operators
' CG_plus,  CG_minus,  CG_mul, CG_div, CG_neg
' CG_power, CG_intDiv, CG_mod, CG_shl, CG_shr

ASSERT (a  +  b ) =   25
ASSERT (b  -  a ) =  -21
ASSERT (a  *  b ) =   46
ASSERT (a  /  b ) =   11
ASSERT (a  \  b ) =   11
ASSERT (b  ^  c ) =    8
ASSERT (a MOD b ) =    1
ASSERT -a         =  -23
ASSERT (b SHL c ) =   16
ASSERT (a SHR c ) =    2

' same but using constants

ASSERT (a  +  bc) =   25
ASSERT (bc +  a ) =   25
ASSERT (bc -  a ) =  -21
ASSERT (ac +  bc) =   25
ASSERT (a  -  bc) =   21
ASSERT (bc -  ac) =  -21
ASSERT (a  *  bc) =   46
ASSERT (bc *  a ) =   46
ASSERT (ac *  b ) =   46
ASSERT (ac *  bc) =   46
ASSERT (a  /  bc) =   11
ASSERT (ec /  b ) =   32
ASSERT (ac /  bc) =   11
ASSERT (a  \  bc) =   11
ASSERT (ec \  b ) =   32
ASSERT (ac \  bc) =   11
ASSERT (bc ^  c ) =    8
ASSERT (d  ^  bc) =   49
ASSERT (bc ^  cc) =    8
ASSERT (a MOD bc) =    1
ASSERT (ac MOD b) =    1
ASSERT (ac MOD bc)=    1
ASSERT (b SHL cc) =   16
ASSERT (bc SHL c) =   16
ASSERT (bc SHL cc)=   16
ASSERT (a SHR cc) =    2
ASSERT (ac SHR c) =    2
ASSERT (ac SHR cc)=    2

' test ADD optimizations

ASSERT a + 0  =   23   : REM identity
ASSERT a + 1  =   24   : REM ADDQ
ASSERT a + 8  =   31   : REM ADDQ

ASSERT 0 + a  =   23   : REM identity
ASSERT 1 + a  =   24   : REM ADDQ
ASSERT 8 + a  =   31   : REM ADDQ

' test SUB optimizations

ASSERT a - 0  =   23   : REM identity
ASSERT a - 1  =   22   : REM SUBQ
ASSERT a - 8  =   15   : REM SUBQ

ASSERT 0 - a  =  -23   : REM identity

' test MUL optimizations

ASSERT a*0   = 0
ASSERT a*1   = 23
ASSERT a*2   = 46
ASSERT a*4   = 92

ASSERT 0  *a = 0
ASSERT 1  *a = 23
ASSERT 2  *a = 46
ASSERT 4  *a = 92

' test POWER optimizations

ASSERT a^0 = 1
ASSERT a^1 = 23

' logical operators
' CG_xor,   CG_eqv,    CG_imp, CG_not, CG_and, CG_or,

ASSERT ( c XOR d ) =  4
ASSERT ( d XOR d ) =  0
ASSERT ( d XOR 1 ) =  6
ASSERT ( 3 XOR d ) =  4
ASSERT (cc XOR dc) =  4
ASSERT ( c EQV d ) = -5
ASSERT ( d EQV d ) = -1
ASSERT ( d EQV 1 ) = -7
ASSERT ( 3 EQV d ) = -5
ASSERT (cc EQV dc) = -5
ASSERT ( c IMP d ) = -1
ASSERT ( d IMP d ) = -1
ASSERT ( d IMP 1 ) = -7
ASSERT ( 3 IMP d ) = -1
ASSERT (cc IMP dc) = -1
ASSERT ( NOT c   ) = -4
ASSERT ( NOT d   ) = -8
ASSERT ( NOT cc  ) = -4
ASSERT ( c AND d ) =  3
ASSERT ( d AND d ) =  7
ASSERT ( d AND 1 ) =  1
ASSERT ( 3 AND d ) =  3
ASSERT (cc AND dc) =  3
ASSERT ( c OR d  ) =  7
ASSERT ( d OR d  ) =  7
ASSERT ( d OR 11 ) = 15
ASSERT ( 3 OR d  ) =  7
ASSERT (cc OR dc ) =  7

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

DIM AS BOOLEAN  mybool = TRUE
DIM AS SINGLE   f  = 42
DIM AS UBYTE    ub = 42
DIM AS INTEGER  i  = 42
DIM AS UINTEGER ui = 42
DIM AS LONG     l  = 42
DIM AS ULONG    ul = 42

b = mybool
ASSERT b = -1
b = f
ASSERT b = 42
b = ub
ASSERT b = 42
b = i
ASSERT b = 42
b = ui
ASSERT b = 42
b = l
ASSERT b = 42
b = ul
ASSERT b = 42

