REM
REM Load a SPRITE from an IFF ILBM image
REM

OPTION EXPLICIT
IMPORT AnimSupport

SCREEN 1, 640, 200, 3, AS_MODE_HIRES, "Sprite Demo 2"
WINDOW 1, "SPRITE Demo 2 - IFF ILBM"

REM load a sprite from an ILBM file

DIM AS SPRITE_t PTR sp

ILBM LOAD SPRITE "PROGDIR:imgs/banana1.iff", sp

REM display it as hardware sprite #1

SPRITE SHOW 1, sp

REM draw something on the background

COLOR 3

FOR x AS INTEGER = 10 TO 630 STEP 3
    LINE (x, 10)-(320,190)
NEXT x

COLOR 1

LOCATE 12, 26
PRINT "Press mouse button to quit"

REM main loop: sprite animation, event handling happens during VWAIT

SUB doQuit (BYVAL wid AS INTEGER, BYVAL button AS BOOLEAN, BYVAL mx AS INTEGER, BYVAL my AS INTEGER, BYVAL ud AS ANY PTR)
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

DIM AS INTEGER vx=1, vy=1
DIM AS INTEGER x=100, y=100

WHILE TRUE
    VWAIT
    SPRITE MOVE 1, (x, y)

    x = x + vx
    y = y + vy

    IF (x>500) OR (x<10) THEN
        vx = -vx
    END IF

    IF (y>170) OR (y<20) THEN
        vy = -vy
    END IF

WEND

