OPTION EXPLICIT

CONST AS INTEGER N = 3

DIM AS SINGLE pX ( STATIC N ), pY ( STATIC N )
DIM AS SINGLE qX ( STATIC N ), qY ( STATIC N )
DIM AS SINGLE rX ( STATIC N ), rY ( STATIC N )

pX ( 0 ) = 10 : pY ( 0 ) = 10
pX ( 1 ) = 600 : pY ( 1 ) = 10
pX ( 2 ) = 50 : pY ( 2 ) = 150
pX ( 3 ) = 500 : pY ( 3 ) = 130

CONST AS SINGLE DELTA = 0.05

DIM AS SINGLE Xold = pX ( 0 ), Yold = pY ( 0 )

DIM AS SINGLE t = - DELTA

WINDOW 1, "Bezier Curve Demo"

FOR i AS INTEGER = 0 TO N
    LINE ( pX ( i ) -4, pY ( i ) -4 ) - ( pX ( i ) + 4, pY ( i ) + 4 ), 3, B
NEXT i

WHILE t < 1
    t = t + DELTA
    DIM AS INTEGER m = N
    FOR i AS INTEGER = 0 TO m
        qX ( i ) = pX ( i )
        qY ( i ) = pY ( i )
    NEXT i
    WHILE m > 0
        FOR j AS INTEGER = 0 TO m -1
            rX ( j ) = qX ( j ) + t * ( qX ( j + 1 ) - qX ( j ) )
            rY ( j ) = qY ( j ) + t * ( qY ( j + 1 ) - qY ( j ) )
        NEXT j
        m = m -1
        FOR j AS INTEGER = 0 TO m
            qX ( j ) = rX ( j )
            qY ( j ) = rY ( j )
        NEXT j
    WEND
    REM PRINT Xold; "/"; Yold; " - "; Qx ( 0 ); "/"; Qy ( 0 )
    LINE ( Xold, Yold ) - ( qX ( 0 ), qY ( 0 ) )
    
    Xold = qX ( 0 ) : Yold = qY ( 0 )
WEND

LOCATE 22, 1

PRINT "PRESS ANY KEY"

WHILE INKEY$ = ""
    SLEEP
WEND

