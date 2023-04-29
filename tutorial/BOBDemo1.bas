REM
REM Create a BOB and display it
REM

OPTION EXPLICIT
IMPORT AnimSupport

WINDOW 1, "BOB Demo"

REM important: initialize the animation system first

GELS INIT

REM create a bitmap, draw into it and create the BOB

DIM AS BITMAP_t PTR bm = NULL

bm = BITMAP(32, 32, 2, TRUE)

BITMAP OUTPUT bm

LINE (0,0)-(31,31),2,B
LINE (1,1)-(30,30),3,BF
COLOR 1,,,DRMD_JAM1 : LOCATE XY (4,18) : PRINT "BOB"

WINDOW OUTPUT 1

DIM AS BOB_t PTR gbob

gbob = BOB (bm)

BOB SHOW gbob

REM draw something on the background

COLOR 3

FOR x AS INTEGER = 10 TO 630 STEP 3
    LINE (x, 10)-(320,190)
NEXT x

COLOR 1

LOCATE 12, 26
PRINT "Press mouse button to quit"

REM main loop: bob animation, event handling happens during VWAIT

SUB doQuit (BYVAL wid AS INTEGER, BYVAL button AS BOOLEAN, BYVAL mx AS INTEGER, BYVAL my AS INTEGER, BYVAL ud AS ANY PTR)
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

DIM AS INTEGER vx=1, vy=1
DIM AS INTEGER x=100, y=100

WHILE TRUE
    VWAIT
    BOB MOVE gbob, (x, y)

    x = x + vx
    y = y + vy

    IF (x>500) OR (x<10) THEN
        vx = -vx
    END IF

    IF (y>170) OR (y<20) THEN
        vy = -vy
    END IF

    GELS REPAINT
WEND

