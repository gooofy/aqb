' ****************************************************
' ** test AND+OR+NOT (short circtuit) evaluation    **
' ****************************************************

OPTION EXPLICIT

DIM AS BOOLEAN a=TRUE, b=TRUE, c=TRUE
DIM AS BOOLEAN d=FALSE, e=FALSE

DIM AS INTEGER i=1, j=2, k=3, l=4

'' _debug_cls

' simple NOT cond

IF NOT (i<j) THEN
    '' _debug_puts "FAIL" : ' _debug_putnl
    ASSERT FALSE
ELSE
    '' _debug_puts "OK" : ' _debug_putnl
    ASSERT TRUE
END IF

ASSERT NOT (i>j)

' bitwise NOT

'' _debug_puts2 NOT i : ' _debug_putnl

ASSERT (NOT i) = -2

' NOT + AND

ASSERT NOT (a AND e)

ASSERT a AND (NOT e)

' NOT + OR

ASSERT NOT (d OR e)
ASSERT d OR (NOT e)

' AND + OR

ASSERT ((i<j) AND (k<j)) OR ((j<k) AND (k<l))
ASSERT ((i<j) OR (k<j)) AND ((k<j) OR (k<l))

' NOT + AND + OR

ASSERT NOT (((i<j) AND (k<j)) OR ((j<k) AND (l<k)))
ASSERT NOT (((j<i) OR (k<j)) AND ((k<j) OR (l<k)))

