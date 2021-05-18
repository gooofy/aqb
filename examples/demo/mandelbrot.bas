OPTION EXPLICIT

CONST AS SINGLE xStart = -2
CONST AS SINGLE yStart = -1.4
CONST AS SINGLE delta = 3.1 / 199
CONST AS SINGLE limes = 4
CONST AS INTEGER maxDepth = 64


SUB CalcMandelbrot (BYVAL xStart AS SINGLE, BYVAL yStart AS SINGLE, BYVAL delta AS SINGLE, BYVAL limes AS SINGLE, BYVAL maxDepth AS INTEGER)

    REM SHARED xStart, yStart, delta, limes, maxDepth%

    DIM AS SINGLE xc = xStart
    FOR xi AS INTEGER = 0 TO 319
        DIM AS SINGLE yc = yStart
        FOR yi AS INTEGER = 0 TO 199
            DIM AS SINGLE xm = xc
            DIM AS SINGLE ym = yc
            DIM AS INTEGER depth = 0
            DIM AS SINGLE xx, yy
            DO
                xx = xm * xm
                yy = ym * ym
                ym = 2 * xm * ym + yc
                xm = xx - yy + xc
                depth = depth + 1
            LOOP UNTIL ( depth = maxDepth ) OR ( xx + yy > limes )
            PSET ( xi, yi ), depth
            yc = yc + delta
        NEXT yi
        xc = xc + delta / 1.8
    NEXT xi

END SUB

SUB setColor ( BYVAL c AS INTEGER, BYVAL r AS SINGLE, BYVAL g AS SINGLE, BYVAL B AS SINGLE )

    WHILE r > 1
        r = r - 1
    WEND

    WHILE g > 1
        g = g - 1
    WEND

    WHILE b > 1
        b = b - 1
    WEND

    PALETTE c, r, g, b
END SUB

SCREEN 1, 320, 200, 5, 1
WINDOW 1, "Mandelbrot Set"

PALETTE 0, 0, 0, 0
PALETTE 1, 1, 1, 1

FOR c AS INTEGER = 2 TO 31
    DIM AS SINGLE r = 0.05 * c, g = 0.1 * c, B = 0.2 * c
    REM PRINT c, r, g, B
    setColor c, r, g, B
NEXT c

CalcMandelbrot xStart, yStart, delta, limes, maxDepth

WHILE INKEY$ = ""
    SLEEP
WEND



