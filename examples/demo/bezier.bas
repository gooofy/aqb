OPTION EXPLICIT

CONST AS INTEGER N=3, BS=4

CONST AS SINGLE DELTA = 0.05

DIM SHARED AS SINGLE pX ( STATIC N ), pY ( STATIC N )
DIM SHARED AS SINGLE qX ( STATIC N ), qY ( STATIC N )
DIM SHARED AS SINGLE rX ( STATIC N ), rY ( STATIC N )

FUNCTION pointhit ( BYVAL mx AS INTEGER, BYVAL my AS INTEGER ) AS INTEGER
    FOR i AS INTEGER = 0 TO N
        'TRACE i; ": pX/pY="; pX ( i ); "/"; pY ( i ); " vs mx/my="; mx; "/"; my; "   "
        IF ( pX(i) - BS ) <= mx AND ( pX(i) + BS ) >= mx THEN
            'TRACE "xhit"
            IF pY(i) - BS <= my AND pY(i) + BS >= my THEN
                'TRACE "yhit"
                RETURN i
            END IF
        END IF
    NEXT i
    RETURN -1
END FUNCTION

SUB drawBezier

    DIM AS SINGLE Xold = pX ( 0 ), Yold = pY ( 0 )

    DIM AS SINGLE t = - DELTA

    CLS

    FOR i AS INTEGER = 0 TO N
        LINE (pX(i)-BS, pY(i)-BS) - (pX(i)+BS, pY(i)+BS), 3, B
    NEXT i

    ' tangents

    LINE (pX(0),pY(0)) - (pX(1),pY(1)), 3
    LINE (pX(3),pY(3)) - (pX(2),pY(2)), 3

    WHILE t < 1
        t = t + DELTA
        DIM AS INTEGER m = N
        FOR i AS INTEGER = 0 TO m
            qX(i) = pX(i)
            qY(i) = pY(i)
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



        LINE ( Xold, Yold ) - ( qX(0), qY(0) )

        Xold = qX(0) : Yold = qY(0)
    WEND

END SUB

DIM SHARED AS INTEGER phit = -1

SUB mousecb (BYVAL wid AS INTEGER, BYVAL button AS BOOLEAN, BYVAL mx AS INTEGER, BYVAL my AS INTEGER, BYVAL ud AS ANY PTR)

    'TRACE "mousecb, MOUSE(0)=";MOUSE(0)

    IF MOUSE(0) < 0 THEN

        phit = pointhit ( mx, my )

        'LOCATE 1, 1
        'TRACE "MOUSE(0)<0, phit="; phit; ",mx="; mx; ", my="; my

    ELSE

        'TRACE "MOUSE(0)>=0, phit="; phit

        IF phit >= 0 THEN

            pX(phit) = mx
            pY(phit) = my

            phit = -1

            drawBezier

        END IF

    END IF
END SUB

SUB mousemovecb (BYVAL wid AS INTEGER, BYVAL button AS BOOLEAN, BYVAL mx AS INTEGER, BYVAL my AS INTEGER, BYVAL ud AS ANY PTR)

    IF phit >= 0 THEN

        IF (mx>BS) AND (my>BS) THEN

            pX(phit) = mx
            pY(phit) = my

            drawBezier
        END IF
    END IF

END SUB

SUB windowcb (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    'TRACE "windowcb"
    SYSTEM
END SUB

pX (0) =  10 : pY (0) =  10
pX (1) = 600 : pY (1) =  10
pX (2) =  50 : pY (2) = 150
pX (3) = 500 : pY (3) = 130

WINDOW 1, "Bezier Curve Demo"

ON MOUSE CALL mousecb
MOUSE ON

ON MOUSE MOTION CALL mousemovecb
MOUSE MOTION ON

ON WINDOW CLOSE CALL 1, windowcb

drawBezier

WHILE INKEY$ = ""
    SLEEP
WEND

