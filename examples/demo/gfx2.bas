'
' some simple 3D figures (parallel projection)
'

OPTION EXPLICIT

WINDOW 2, "gfx2 - Simple 3D Graphics"

' cylinder

FOR w AS SINGLE = 0 TO PI STEP 0.04 * PI
    DIM x AS SINGLE = 20 * SIN ( w )
    DIM y AS SINGLE = 35 * COS ( w )
    LINE ( x + 50, y + 52 ) - ( x + 200, y + 52 )
NEXT
FOR w AS SINGLE = 0 TO 2 * PI STEP 0.02 * PI
    DIM x AS SINGLE = 20 * SIN ( w )
    DIM y AS SINGLE = 35 * COS ( w )
    PSET ( x + 50, y + 52 )
    IF w < PI THEN PSET ( x + 200, y + 52 )
NEXT

' paraboloid

FOR z AS SINGLE = -3 TO 3 STEP 0.1
    DIM a AS SINGLE = 0.5 * z
    DIM z2 AS SINGLE = z * z
    FOR x AS SINGLE = -3 TO 3.2 STEP 0.2
        DIM y AS SINGLE = 0.5 * ( z2 + x * x )
        PSET ( 430 + 30 * ( x + a ), 90 -8 * ( y - a ) )
    NEXT x
NEXT z

' pyramid

CONST n AS SINGLE = 5
CONST AS SINGLE x = 130, y = 92, d = 2 * PI / n
FOR w AS SINGLE = 0 TO 2 * PI STEP d
    DIM AS SINGLE x1 = 130 + 80 * COS ( w )
    DIM AS SINGLE y1 = 56 + 15 * SIN ( w )
    IF w = 0 THEN PSET ( 210, y + 56 ) ELSE LINE - ( x1, y + y1 )
    PSET ( x, y ) : LINE - ( x1, y + y1 )
NEXT

' sinusoid
LINE ( 380, 182 ) - ( 580, 182 )
LINE ( 380, 182 ) - ( 380, 102 )
LINE ( 380, 182 ) - ( 550, 127 )
LOCATE 23, 45 : PRINT "0";
LOCATE 23, 76 : PRINT "X";
LOCATE 13, 45 : PRINT "Y"
LOCATE 16, 71 : PRINT "Z"
FOR w AS SINGLE = 0 TO 4 * PI STEP PI / 5
    DIM AS SINGLE z = 6 * w, x = 80 * SIN ( w )
    IF w = 0 THEN
        PSET ( 380 + 2.2 * z, 182 -0.707 * z )
    ELSE
        LINE - ( 380 + x + 2.2 * z, 182 -0.707 * z )
    END IF
NEXT

LOCATE 22, 1
PRINT "PRESS ANY KEY TO QUIT"

WHILE INKEY$ ( ) = ""
    SLEEP
WEND

