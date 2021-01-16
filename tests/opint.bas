a% = 23
b% = 42
c% = 3 : d% = 7

' arithemtic operators
' CG_plus,  CG_minus,  CG_mul, CG_div, CG_neg
' CG_power, CG_intDiv, CG_mod, CG_shl, CG_shr

ASSERT (a%  +  b% ) =   65
ASSERT (a%  -  b% ) =  -19
ASSERT (a%  *  b% ) =  966
ASSERT (b%  /  a% ) =    1
ASSERT (b%  \  a% ) =    1
ASSERT (a%  ^  c% ) =12167
ASSERT (b% MOD a% ) =   19
ASSERT  -a%         =  -23
ASSERT ( c% SHL d%) =  384
ASSERT ( b% SHR c%) =    5

' same but using constants

ASSERT (a% + 42  )  =   65
ASSERT (42 + a%  )  =   65
ASSERT (23 + 42  )  =   65
ASSERT (a% - 42  )  =  -19
ASSERT (42 - a%  )  =   19
ASSERT (42 - 23  )  =   19
ASSERT (23 - 42  )  =  -19
ASSERT (a% * 42  )  =  966
ASSERT (42 * a%  )  =  966
ASSERT (42 * 23  )  =  966
ASSERT (b% /  3  )  =   14
ASSERT (92 / a%  )  =    4
ASSERT (92 / 23  )  =    4
ASSERT (b% \  3  )  =   14
ASSERT (92 \ a%  )  =    4
ASSERT (92 \ 23  )  =    4
ASSERT (a% ^  3  )  =12167
ASSERT (4  ^ c%  )  =   64
ASSERT (23 ^  3  )  =12167
ASSERT (b% MOD 23)  =   19
ASSERT (42 MOD a%)  =   19
ASSERT (42 MOD 23)  =   19
ASSERT  -(23)       =  -23
ASSERT ( c% SHL  7) =  384
ASSERT (  3 SHL d%) =  384
ASSERT ( 3  SHL  7) =  384
ASSERT ( 42 SHR c%) =    5
ASSERT ( b% SHR  3) =    5
ASSERT ( 42 SHR  3) =    5

' test ADD optimizations

ASSERT b% + 0  =   42   : REM identity
ASSERT b% + 1  =   43   : REM ADDQ
ASSERT b% + 8  =   50   : REM ADDQ

ASSERT 0 + b%  =   42   : REM identity
ASSERT 1 + b%  =   43   : REM ADDQ
ASSERT 8 + b%  =   50   : REM ADDQ

' test SUB optimizations


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

' test POWER optimizations

ASSERT b%^0 = 1
ASSERT b%^1 = 42

' logical operators
' CG_xor,   CG_eqv,    CG_imp, CG_not, CG_and, CG_or,

ASSERT ( c% XOR d% ) =  4
ASSERT ( d% XOR d% ) =  0
ASSERT ( d% XOR 1  ) =  6
ASSERT ( 3  XOR d% ) =  4
ASSERT ( 3  XOR  7 ) =  4
ASSERT ( c% EQV d% ) = -5
ASSERT ( d% EQV d% ) = -1
ASSERT ( d% EQV 1  ) = -7
ASSERT (  3 EQV d% ) = -5
ASSERT (  3 EQV 7  ) = -5
ASSERT ( c% IMP d% ) = -1
ASSERT ( d% IMP d% ) = -1
ASSERT ( d% IMP 1  ) = -7
ASSERT (  3 IMP d% ) = -1
ASSERT (  3 IMP 7  ) = -1
ASSERT ( NOT c%    ) = -4
ASSERT ( NOT d%    ) = -8
ASSERT ( NOT  7    ) = -8
ASSERT ( c% AND  0 ) =  0
ASSERT (  0 AND d% ) =  0
ASSERT ( c% AND d% ) =  3
ASSERT ( d% AND d% ) =  7
ASSERT ( d% AND 1  ) =  1
ASSERT (  3 AND d% ) =  3
ASSERT (  3 AND 7  ) =  3
ASSERT ( c% OR d%  ) =  7
ASSERT ( d% OR d%  ) =  7
ASSERT ( d% OR 11  ) = 15
ASSERT (  3 OR d%  ) =  7
ASSERT ( c% OR  0  ) =  3
ASSERT (  0 OR d%  ) =  7
ASSERT (  3 OR 7   ) =  7

' relational operators (relOp)

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

' relOp constant optimization tests

ASSERT  3 =  3
ASSERT NOT (3 =  4)
ASSERT  3 <> 4
ASSERT NOT (3 <> 3)
ASSERT  3 < 4
ASSERT NOT (3 < 3)
ASSERT  4  > 3
ASSERT NOT (3  > 3)
ASSERT  3 <= 4
ASSERT NOT (3 <= 2)
ASSERT  4 >= 3
ASSERT NOT (3 >= 4)

' conversion tests

fT1  = 25000.0
lT1& = 10000

i% = fT1
ASSERT i% = 25000
i% = lT1& > 23
ASSERT i% = -1
i% = lT1&
ASSERT i% = 10000

