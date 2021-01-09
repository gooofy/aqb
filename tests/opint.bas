a% = 23
b% = 42

' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp

ASSERT (a%  +  b% ) =   65
ASSERT (a%  -  b% ) =  -19
ASSERT (a%  *  b% ) =  966
ASSERT (b%  /  a% ) =    1
ASSERT (a%  ^   3 ) =12167
ASSERT (b%  \  a% ) =    1
ASSERT (b% MOD a% ) =   19

' same but using constants

ASSERT (a% + 42  ) =   65
ASSERT (42 + a%  ) =   65
ASSERT (a% - 42  ) =  -19
ASSERT (42 - a%  ) =   19
ASSERT (a% * 42  ) =  966
ASSERT (42 * a%  ) =  966
ASSERT (b% / 23  ) =    1
ASSERT (b% MOD 23) =   19

' test ADD optimizations

ASSERT 23 + 42 = 65

ASSERT b% + 0  =   42   : REM identity
ASSERT b% + 1  =   43   : REM ADDQ
ASSERT b% + 8  =   50   : REM ADDQ

ASSERT 0 + b%  =   42   : REM identity
ASSERT 1 + b%  =   43   : REM ADDQ
ASSERT 8 + b%  =   50   : REM ADDQ

' test SUB optimizations

ASSERT 42 - 23 = 19
ASSERT 23 - 42 = -19

ASSERT b% - 0  =   42   : REM identity
ASSERT b% - 1  =   41   : REM SUBQ
ASSERT b% - 8  =   34   : REM SUBQ

ASSERT 0 - b%  =  -42   : REM identity

' test MUL optimizations

ASSERT b%*0   = 0
ASSERT b%*1   = 42
ASSERT b%*2   = 84
ASSERT b%*4   = 168
ASSERT b%*8   = 336
ASSERT b%*16  = 672
ASSERT b%*32  = 1344
ASSERT b%*64  = 2688
ASSERT b%*128 = 5376
ASSERT b%*256 = 10752

ASSERT 0  *b% = 0
ASSERT 1  *b% = 42
ASSERT 2  *b% = 84
ASSERT 4  *b% = 168
ASSERT 8  *b% = 336
ASSERT 16 *b% = 672
ASSERT 32 *b% = 1344
ASSERT 64 *b% = 2688
ASSERT 128*b% = 5376
ASSERT 256*b% = 10752

' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp

c% = 3 : d% = 7

ASSERT ( c% XOR d% ) =  4
ASSERT ( d% XOR d% ) =  0
ASSERT ( d% XOR 1  ) =  6
ASSERT ( c% EQV d% ) = -5
ASSERT ( d% EQV d% ) = -1
ASSERT ( d% EQV 1  ) = -7
ASSERT ( c% IMP d% ) = -1
ASSERT ( d% IMP d% ) = -1
ASSERT ( d% IMP 1  ) = -7
ASSERT ( NOT c%    ) = -4
ASSERT ( NOT d%    ) = -8
ASSERT ( c% AND d% ) =  3
ASSERT ( d% AND d% ) =  7
ASSERT ( d% AND 1  ) =  1
ASSERT ( c% OR d%  ) =  7
ASSERT ( d% OR d%  ) =  7
ASSERT ( d% OR 11  ) = 15

ASSERT ( c% SHL d%) = 384
ASSERT ( b% SHR c%) =   5

ASSERT ( 1  SHL d%) = 128
ASSERT (128 SHR c%) =  16

' relational operators

ASSERT  a% =  a%
ASSERT NOT (a% =  b%)
ASSERT  a% <> b%
ASSERT NOT (a% <> a%)
ASSERT  a% <  b%
ASSERT NOT (a% <  a%)
ASSERT NOT (b% <  a%)
ASSERT NOT (a% >  b%)
ASSERT NOT (a% >  a%)
ASSERT b% >  a%
ASSERT a% <= b%
ASSERT a% <= a%
ASSERT NOT (b% <= a%)
ASSERT NOT (a% >= b%)
ASSERT  a% >= a%
ASSERT  b% >= a%

' conversion tests

fT1  = 25000.0
lT1& = 10000

i% = fT1
ASSERT i% = 25000
i% = lT1& > 23
ASSERT i% = -1
i% = lT1&
ASSERT i% = 10000

