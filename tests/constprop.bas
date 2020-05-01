' test constant propagation

' bool

ASSERT ((TRUE XOR FALSE) = TRUE)
ASSERT ((TRUE EQV TRUE) = TRUE)
ASSERT ((FALSE IMP TRUE) = TRUE)
ASSERT (NOT FALSE = TRUE)
ASSERT ((TRUE AND TRUE) = TRUE)
ASSERT ((TRUE OR FALSE) = TRUE)


' integer / long

ASSERT (23 + 19 = 42)
ASSERT (42 - 19 = 23)
ASSERT ( 3 *  3 =  9)
ASSERT ( 9 /  3 =  3)
ASSERT (37 MOD 5 = 2)
ASSERT (15 EQV 7 = 7)
ASSERT (0 IMP 7 = 7)
ASSERT (-23 = -23)
ASSERT (NOT 7 = -8)

ASSERT ((1 AND 1) = 1)
ASSERT ((1 OR  0) = 1)

ASSERT ( 2^3 = 8 )
ASSERT ( 9 \  3 =  3)

' single precision floats

ASSERT (23.0 + 19.0 = 42)
ASSERT (42.0 - 19.0 = 23)
ASSERT ( 3.0 *  3.0 =  9)
ASSERT ( 9.0 /  3.0 =  3)
ASSERT (37.0 MOD 5.0 = 2)
ASSERT (15.0 EQV 7.0 = 7)
ASSERT (0.0 IMP 7.0 = 7)
ASSERT (-23.0 = -23.0)
ASSERT (NOT 7.6 = -8 )

ASSERT ((7.3 AND 1.5) = 2)
ASSERT ((7.3 OR  2.5) = 3)

ASSERT ( 2.0^3.0 = 8.0 )
ASSERT ( 9.0 \  3.0 =  3.0)

