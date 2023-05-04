REM
REM Create a SPRITE and display it
REM

OPTION EXPLICIT
IMPORT AnimSupport

SCREEN 1, 640, 200, 3, AS_MODE_HIRES, "Sprite Demo"
WINDOW 1, "SPRITE Demo"

REM create a bitmap, draw into it and create the SPRITE

DIM AS BITMAP_t PTR bm = NULL

bm = BITMAP(16, 16, 2)

BITMAP OUTPUT bm

LINE (0,0)-(15,15),0,BF
LINE (0,0)-(15,15),2,B
COLOR 1,,,DRMD_JAM1 : LOCATE XY (0,8) : PRINT "SP"

WINDOW OUTPUT 1

DIM AS SPRITE_t PTR sp

sp = SPRITE (bm)

REM use hardware sprite #1

SPRITE SHOW 1, sp

REM background

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

