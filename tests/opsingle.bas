a = 3.14
b = 3.14e+2
c = 3.14e-2
d = 3! : e = 7!

CONST AS SINGLE bc = 3.14e+2

' PRINT "a ="; a
' PRINT "b ="; b
' PRINT "c ="; c

' arithemtic operators
' CG_plus,  CG_minus,  CG_mul, CG_div, CG_neg
' CG_power, CG_intDiv, CG_mod, CG_shl, CG_shr

ASSERT (a  +  b)         =    317.14
ASSERT (a  +  c)         =     3.1714
ASSERT (a  -  b)         =  -310.86
ASSERT (a  *  b)         =   985.96
ASSERT (b  /  a)         =   100.00
ASSERT INT((e  /  d)*1000.0) =   2333
ASSERT (b  \  a)         =   104.00
ASSERT INT((e  \  d)*1000.0) =   2000
ASSERT INT((b^c)*1000.0) =  1198
ASSERT (b MOD a)         =     2
ASSERT -b                =  -314
ASSERT INT(d SHL e)      =   384
ASSERT INT(b SHR d)      =    39

' same but using constants

ASSERT (      a  +  3.14e+2)   =  317.14
ASSERT (   3.14  +        b)   =  317.14
ASSERT (      a  +  3.14e-2)   =  3.1714
ASSERT (   3.14  +        c)   =  3.1714
ASSERT (      a  -  3.14e+2)   = -310.86
ASSERT (   3.14  -        b)   = -310.86
ASSERT (      a  *  3.14e+2)   =  985.96
ASSERT (   3.14  *        b)   =  985.96
ASSERT (3.14e+2  /        a)   =  100.00
ASSERT (      b  /     3.14)   =  100.00
ASSERT INT((e  / 3!)*1000.0) =   2333
ASSERT INT((7! /  d)*1000.0) =   2333
ASSERT (3.14e+2  \        a)   =  104.00
ASSERT (      b  \     3.14)   =  104.00
ASSERT INT((e  \ 3!)*1000.0) =   2000
ASSERT INT((7! \  d)*1000.0) =   2000
ASSERT INT((3.14e+2^c)*1000.0) = 1198
ASSERT INT((b^3.14e-2)*1000.0) = 1198
ASSERT (3.14e+2 MOD       a)   =    2
ASSERT (      b MOD    3.14)   =    2
ASSERT INT(3 SHL e)            =  384
ASSERT INT(d SHL 7)            =  384
ASSERT INT(b SHR 3)            =   39
ASSERT INT(3.14e+2 SHR d)      =   39

ASSERT CLNG((    3.14  +  3.14e+2)*1000) =  317140
ASSERT CLNG((    3.14  +  3.14e-2)*1000) =    3171
ASSERT CLNG((    3.14  -  3.14e+2)*1000) = -310860
ASSERT CLNG((    3.14  *  3.14e+2)*1000) =  985960
ASSERT CLNG(( 3.14e+2  /  3.14   )*1000) =  100000
ASSERT INT((7! /  3!)*1000.0) =   2333
ASSERT CLNG(( 3.14e+2  ^  3.14e-2)*1000) =    1198
ASSERT CLNG(( 3.14e+2  \  3.14   )*1000) =  104000
ASSERT INT((7! \ 3!)*1000.0) =   2000
ASSERT CLNG(( 3.14e+2 MOD 3.14   )*1000) =    2000
ASSERT CLNG(( -bc   )*1000) =    -314000
ASSERT CLNG((      3! SHL 7!     )*1000) =  384000
ASSERT CLNG(( 3.14e+2 SHR 3!     )*1000) =   39000

' logical operators
' CG_xor,   CG_eqv,    CG_imp, CG_not, CG_and, CG_or,

ASSERT ( d  XOR  e ) =  4
ASSERT ( e  XOR  e ) =  0
ASSERT ( e  XOR 1! ) =  6
ASSERT (2.5 XOR  e ) =  4

ASSERT ( d  EQV  e ) = -5
ASSERT ( e  EQV  e ) = -1
ASSERT ( e  EQV 1! ) = -7
ASSERT (2.5 EQV  e ) = -5

ASSERT (  d IMP  e ) = -1
ASSERT (  e IMP  e ) = -1
ASSERT (  e IMP 0.5) = -7
ASSERT (2.5 IMP  e ) = -1

ASSERT ( NOT d   ) = -4
ASSERT ( NOT e   ) = -8
ASSERT ( NOT 2.5 ) = -4

ASSERT (  d AND e  ) =  3
ASSERT (  e AND e  ) =  7
ASSERT (  e AND 1! ) =  1
ASSERT (2.5 AND e  ) =  3

ASSERT (  d OR  e  ) =  7
ASSERT (  e OR  e  ) =  7
ASSERT (  e OR 11! ) = 15
ASSERT (2.5 OR  e  ) =  7

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

' relOp constant optimization tests

ASSERT  3! =  3!
ASSERT NOT (3! =  4!)
ASSERT  3! <> 4!
ASSERT NOT (3! <> 3!)
ASSERT  3! < 4!
ASSERT NOT (3! < 3!)
ASSERT  4!  > 3!
ASSERT NOT (3!  > 3!)
ASSERT  3! <= 4!
ASSERT NOT (3! <= 2)
ASSERT  4! >= 3!
ASSERT NOT (3! >= 4!)

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

