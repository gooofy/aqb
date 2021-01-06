' *********************************************
' ** test OR (short circtuit) evaluation    **
' *********************************************

OPTION EXPLICIT

DIM AS BOOLEAN a=TRUE, b=TRUE, c=TRUE
DIM AS BOOLEAN d=FALSE, e=FALSE

DIM AS INTEGER i=1, j=2, k=3, l=4

' _debug_cls

' OR short-circuit evaluation

IF (j<i) OR (j<k) THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF

IF d OR b THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF

' test ELSE

IF (j<i) OR (j>k) THEN
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
ELSE
    ' _debug_puts "OK" : ' _debug_putnl
    ASSERT TRUE
END IF

ASSERT (j<i) OR (j<k)
' will fail: ASSERT (i<j) OR (j>k)

IF e OR d THEN
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
ELSE
    ' _debug_puts "OK" : ' _debug_putnl
    ASSERT TRUE
END IF

ASSERT a OR e

' 3 x OR short-circuit chain
IF (i<j) OR (k<j) OR (k<l) THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF

ASSERT (i<j) OR (k<j) OR (k<l)

IF d OR e OR c THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF

ASSERT d OR e OR c

' test cond -> int evaluation

' _debug_puts2 (i<j) OR k : ' _debug_putnl
' _debug_puts2 (i>j) OR k : ' _debug_putnl

ASSERT ((i<j) OR k) = -1
ASSERT ((i>j) OR k) = 3

ASSERT (a OR k) = -1
ASSERT (d OR k) = 3

