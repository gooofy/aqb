DIM AS UBYTE a=23, b=2, c=3, d=7
CONST AS UBYTE ac=23, bc=2, cc=3, dc=7

' arithemtic operators
' CG_plus,  CG_minus,  CG_mul, CG_div, CG_neg
' CG_power, CG_intDiv, CG_mod, CG_shl, CG_shr

ASSERT (a  +  b ) =  25
ASSERT (a  -  b ) =  21
ASSERT (a  *  b ) =  46
ASSERT (a  /  b ) =  11
ASSERT (b  ^  c ) =   8
ASSERT (a  \  b ) =  11
ASSERT (a MOD b ) =   1
ASSERT (b SHL c ) =  16
ASSERT (a SHR c ) =   2

' same but using constants

ASSERT (ac  +  b ) =  25
ASSERT (a   +  bc) =  25
ASSERT (ac  +  bc) =  25
ASSERT (ac  -  b ) =  21
ASSERT (a   -  bc) =  21
ASSERT (ac  -  bc) =  21
ASSERT (ac  *  b ) =  46
ASSERT (a   *  bc) =  46
ASSERT (b   *  ac) =  46
ASSERT (ac  *  bc) =  46
ASSERT (ac  /  b ) =  11
ASSERT (a   /  bc) =  11
ASSERT (ac  /  bc) =  11
ASSERT (bc  ^  c ) =   8
ASSERT (b   ^  cc) =   8
ASSERT (bc  ^  cc) =   8
ASSERT (ac  \  b ) =  11
ASSERT (a   \  bc) =  11
ASSERT (ac  \  bc) =  11
ASSERT (ac MOD b ) =   1
ASSERT (a  MOD bc) =   1
ASSERT (ac MOD bc) =   1
ASSERT (bc SHL c ) =  16
ASSERT (b  SHL cc) =  16
ASSERT (bc SHL cc) =  16
ASSERT (ac SHR c ) =   2
ASSERT (a  SHR cc) =   2
ASSERT (ac SHR cc) =   2

' test ADD optimizations

ASSERT b + 0  =    2   : REM identity
ASSERT b + 1  =    3   : REM ADDQ
ASSERT b + 8  =   10   : REM ADDQ

ASSERT 0 + b  =    2   : REM identity
ASSERT 1 + b  =    3   : REM ADDQ
ASSERT 8 + b  =   10   : REM ADDQ

' test SUB optimizations

ASSERT b - 0  =    2   : REM identity
ASSERT b - 1  =    1   : REM SUBQ
ASSERT a - 8  =   15   : REM SUBQ

' test MUL optimizations

ASSERT b*0   = 0
ASSERT b*1   = 2
ASSERT b*2   = 4
ASSERT b*4   = 8
ASSERT b*8   = 16
ASSERT b*16  = 32
ASSERT b*32  = 64
ASSERT b*64  = 128
ASSERT b*128 = 256
ASSERT b*256 = 512

ASSERT 0  *b = 0
ASSERT 1  *b = 2
ASSERT 2  *b = 4
ASSERT 4  *b = 8
ASSERT 8  *b = 16
ASSERT 16 *b = 32
ASSERT 32 *b = 64
ASSERT 64 *b = 128
ASSERT 128*b = 256
ASSERT 256*b = 512

' test POWER optimizations

ASSERT b^0 = 1
ASSERT b^1 = 2

' logical operators
' CG_xor,   CG_eqv,    CG_imp, CG_not, CG_and, CG_or,

ASSERT ( c XOR d ) =  4
ASSERT (cc XOR d ) =  4
ASSERT ( c XOR dc) =  4
ASSERT (cc XOR dc) =  4
ASSERT ( d XOR d ) =  0
ASSERT (dc XOR d ) =  0
ASSERT ( d XOR dc) =  0
ASSERT (dc XOR dc) =  0
ASSERT ( d XOR 1 ) =  6
ASSERT ( c EQV d ) = 251
ASSERT (cc EQV d ) = 251
ASSERT ( c EQV dc) = 251
ASSERT (cc EQV dc) = 251
'_debug_puts2 ( d EQV d ) : _debug_putnl
'_debug_puts2 ( d EQV 1 ) : _debug_putnl
ASSERT ( d EQV d ) = 255
ASSERT ( d EQV 1 ) = -7
' _debug_puts2 c IMP d :debug_putnl
ASSERT ( c IMP d ) = 255
ASSERT (cc IMP d ) = 255
ASSERT ( c IMP dc) = 255
ASSERT (cc IMP dc) = 255
ASSERT ( d IMP d ) = 255
ASSERT ( d IMP 1 ) = -7
ASSERT ( NOT c   ) = 252
ASSERT ( NOT cc  ) = 252
' _debug_puts2 NOT d : _debug_putnl
ASSERT ( NOT d   ) = 248
ASSERT ( c AND d ) =  3
ASSERT (cc AND d ) =  3
ASSERT ( c AND dc) =  3
ASSERT (cc AND dc) =  3
ASSERT ( d AND d ) =  7
ASSERT ( d AND 1 ) =  1
ASSERT ( c OR d  ) =  7
ASSERT (cc OR d  ) =  7
ASSERT ( c OR dc ) =  7
ASSERT (cc OR dc ) =  7
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

