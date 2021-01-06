' *********************************************
' ** test AND (short circtuit) evaluation    **
' *********************************************

OPTION EXPLICIT

DIM AS BOOLEAN a=TRUE, b=TRUE, c=TRUE
DIM AS BOOLEAN d=FALSE

DIM AS INTEGER i=1, j=2, k=3, l=4

' _debug_cls

' AND short-circuit evaluation

IF (i<j) AND (j<k) THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF

IF a AND b THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF


' test ELSE

IF (i<j) AND (j>k) THEN
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
ELSE
    ' _debug_puts "OK" : ' _debug_putnl
    ASSERT TRUE
END IF

ASSERT (i<j) AND (j<k)
' will fail: ASSERT (i<j) AND (j>k)

IF a AND d THEN
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
ELSE
    ' _debug_puts "OK" : ' _debug_putnl
    ASSERT TRUE
END IF

ASSERT a AND b

' 3 x AND short-circuit chain
IF (i<j) AND (j<k) AND (k<l) THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF

ASSERT (i<j) AND (j<k) AND (k<l)

IF a AND b AND c THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF

ASSERT a AND b AND c

' test cond -> int evaluation

' _debug_puts2 (i<j) AND k : ' _debug_putnl

ASSERT ((i<j) AND k) = 3
ASSERT ((i>j) AND k) = 0

ASSERT (a AND k) = 3
ASSERT (d AND k) = 0

' test int -> cond conversion

IF i THEN
    ' _debug_puts "OK" : ' _debug_putnl
ELSE
    ' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
END IF

