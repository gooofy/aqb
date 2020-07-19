OPTION EXPLICIT

' Paraboloid

WINDOW 2, "gfx2 - 3D Graphics"

FOR z AS SINGLE = -3 TO 3 STEP .1
    DIM a AS SINGLE = .5*z
    DIM z2 AS SINGLE = z*z
    FOR x AS SINGLE = -3 TO 3.2 STEP .2
        DIM y AS SINGLE = 0.5 * (z2 + x*x)
        PSET ( 430 + 30*(x+a), 90 - 8*(y-a) )
        ' LOCATE 0,0 : PRINT z, x
    NEXT x
    ' PRINT z, a, z2
NEXT z

LOCATE 20,0
PRINT "PRESS ANY KEY TO QUIT"

WHILE INKEY$() = ""
WEND

' FOR z AS SINGLE = -3 TO 3 STEP .1
'     PRINT z
' NEXT z
