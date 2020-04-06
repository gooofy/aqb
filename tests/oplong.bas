a& = 23
b& = 42

' arithemtic operators
' A_addOp, A_subOp,    A_mulOp, A_divOp,
' A_expOp, A_intDivOp, A_modOp, A_negOp

ASSERT (a&  +  b&) =    65
ASSERT (a&  -  b&) =   -19
ASSERT (a&  *  b&) =   966
ASSERT (b&  /  a&) =     1
ASSERT (a&  ^   3) = 12167
ASSERT (b&  \  a&) =     1
ASSERT (b& MOD a&) =    19

' logical operators
' A_xorOp, A_eqvOp, A_impOp, A_notOp, A_andOp, A_orOp

c& = 3 : d& = 7

ASSERT (c& XOR d&) = 4
ASSERT (d& XOR d&) = 0
ASSERT (d& XOR 1)  = 6

ASSERT (c& EQV d&) =-5
ASSERT (d& EQV d&) =-1
ASSERT (d& EQV 1)  =-7

ASSERT (c& IMP d&) =-1
ASSERT (d& IMP d&) =-1
ASSERT (d& IMP 1)  =-7

ASSERT (NOT c&) =-4
ASSERT (NOT d&) =-8

ASSERT (c& AND d&) = 3
ASSERT (d& AND d&) = 7
ASSERT (d& AND 1)  = 1

ASSERT (c& OR d&) = 7
ASSERT (d& OR d&) = 7
ASSERT (d& OR 11) = 15

' relational operators

ASSERT     a& =  a&
ASSERT NOT(a& =  b&)
ASSERT     a& <> b&
ASSERT NOT(a& <> a&)
ASSERT     a& <  b&
ASSERT NOT(a& <  a&)
ASSERT NOT(b& <  a&)
ASSERT NOT(a& >  b&)
ASSERT NOT(a& >  a&)
ASSERT     b& >  a&
ASSERT     a& <= b&
ASSERT     a& <= a&
ASSERT NOT(b& <= a&)
ASSERT NOT(a& >= b&)
ASSERT     a& >= a&
ASSERT     b& >= a&


' conversion tests

fT1  = 25000.0
iT1% = 10000

i& = fT1
ASSERT i& = 25000
i& = iT1% > 23
ASSERT i& =    -1
i& = iT1%
ASSERT i& = 10000
