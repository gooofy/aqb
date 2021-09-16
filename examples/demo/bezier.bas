OPTION EXPLICIT

DIM AS integer n = 3

CONST AS integer MAX_N = 10

DIM AS single Px ( MAX_N ), Py ( MAX_N ), Qx ( MAX_N ), Qy ( MAX_N )
DIM AS single Rx ( MAX_N ), Ry ( MAX_N )

Px ( 0 ) = 10 : Py ( 0 ) = 10
Px ( 1 ) = 100 : Py ( 1 ) = 42
Px ( 2 ) = 200 : Py ( 2 ) = 23
Px ( 3 ) = 300 : Py ( 3 ) = 100

DIM AS single Delta = 0.01

DIM AS single Xold = Px ( 0 ), Yold = Py ( 0 )

DIM AS single T = - Delta

WHILE T < 1
    T = T + Delta
    DIM AS integer M = n
    FOR i AS integer = 0 TO M
        Qx ( i ) = Px ( i )
        Qy ( i ) = Py ( i )
    NEXT i
    WHILE M > 0
        FOR j AS integer = 0 TO M -1
            Rx ( j ) = Qx ( j ) + T * ( Qx ( j + 1 ) - Qx ( j ) )
            Ry ( j ) = Qy ( j ) + T * ( Qy ( j + 1 ) - Qy ( j ) )
        NEXT j
        M = M -1
        FOR j AS integer = 0 TO M
            Qx ( j ) = Rx ( j )
            Qy ( j ) = Ry ( j )
        NEXT j
    WEND
    REM PRINT Xold; "/"; Yold; " - "; Qx ( 0 ); "/"; Qy ( 0 )
    LINE ( Xold, Yold ) - ( Qx ( 0 ), Qy ( 0 ) )
    
    Xold = Qx ( 0 ) : Yold = Qy ( 0 )
WEND

PRINT "PRESS ANY KEY"

WHILE INKEY$ = ""
    SLEEP
WEND

