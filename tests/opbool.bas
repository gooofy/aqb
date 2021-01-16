
DIM AS BOOLEAN a, b

CONST AS BOOLEAN ac=TRUE, bc=FALSE

a = TRUE
b = FALSE

' logical operators
' CG_xor,   CG_eqv,    CG_imp, CG_not, CG_and, CG_or,

ASSERT a XOR b
ASSERT ( a XOR a )   =  FALSE
ASSERT a XOR FALSE
ASSERT ( a EQV b )   = FALSE
ASSERT b EQV b
ASSERT a EQV TRUE
ASSERT b IMP a
ASSERT ( a IMP b )   = FALSE
ASSERT b IMP TRUE
ASSERT ( NOT a )     = FALSE
ASSERT NOT b
ASSERT NOT FALSE
ASSERT a AND a
ASSERT (a AND b )    = FALSE
ASSERT a AND TRUE
ASSERT (b AND FALSE) = FALSE
ASSERT a OR b
ASSERT NOT (b OR b)
ASSERT NOT (b OR FALSE)
ASSERT b OR TRUE
ASSERT a OR TRUE

' same with constants

ASSERT ac XOR b
ASSERT a XOR bc
ASSERT ac XOR bc
ASSERT ( ac EQV b )   = FALSE
ASSERT ( a EQV bc )   = FALSE
ASSERT ( ac EQV bc )  = FALSE
ASSERT bc IMP a
ASSERT b IMP ac
ASSERT bc IMP ac
ASSERT ( NOT ac )     = FALSE
ASSERT (ac AND b )    = FALSE
ASSERT (a AND bc )    = FALSE
ASSERT (ac AND bc )   = FALSE
ASSERT ac OR b
ASSERT a OR bc
ASSERT ac OR bc

' relational operators

ASSERT a <> B
ASSERT a = a
ASSERT NOT (a = b)
ASSERT NOT (a <> a)

' conversion tests

DIM i AS integer
DIM f AS single
DIM l AS long

i = 42
f = 1.0
l = 23

b = i
ASSERT b
b = f
ASSERT b
b = l
ASSERT b

i = 0
f = 0.0
l = 0

b = i
ASSERT NOT b
b = f
ASSERT NOT b
b = l
ASSERT NOT b
