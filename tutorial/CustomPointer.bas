REM
REM Load a SPRITE from an IFF ILBM image and use it as a custom pointer
REM

OPTION EXPLICIT
IMPORT AnimSupport

WINDOW 1, "Custom Pointer Demo"

REM load a sprite from an ILBM file

DIM AS SPRITE_t PTR sp

ILBM LOAD SPRITE "PROGDIR:imgs/banana1.iff", sp

REM use it as a custom mouse pointer

POINTER SPRITE sp

REM draw something on the background

COLOR 3

FOR x AS INTEGER = 10 TO 630 STEP 3
    LINE (x, 10)-(320,190)
NEXT x

COLOR 1

LOCATE 12, 26
PRINT "Press mouse button to quit"

REM main loop

SUB doQuit (BYVAL wid AS INTEGER, BYVAL button AS BOOLEAN, BYVAL mx AS INTEGER, BYVAL my AS INTEGER, BYVAL ud AS ANY PTR)
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

WHILE TRUE
    SLEEP
WEND


