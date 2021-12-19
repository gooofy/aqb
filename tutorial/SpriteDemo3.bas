REM
REM Sprite animation
REM

OPTION EXPLICIT
IMPORT AnimSupport

WINDOW 1, "SPRITE Demo 3 - Animation"

DIM AS SPRITE_t PTR sp1, sp2, sp3, sp4

ILBM LOAD SPRITE "PROGDIR:imgs/banana1.iff", sp1
ILBM LOAD SPRITE "PROGDIR:imgs/banana2.iff", sp2
ILBM LOAD SPRITE "PROGDIR:imgs/banana3.iff", sp3
ILBM LOAD SPRITE "PROGDIR:imgs/banana4.iff", sp4

REM draw something on the background

COLOR 3

FOR x AS INTEGER = 10 TO 630 STEP 3
    LINE (x, 10)-(320,190)
NEXT x    

COLOR 1

LOCATE 12, 26
PRINT "Press mouse button to quit"

REM main loop: sprite animation, event handling happens during VWAIT

SUB doQuit
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

DIM AS INTEGER vx=1, vy=1
DIM AS INTEGER x=100, y=100

DIM AS INTEGER spmov=0

WHILE TRUE
    VWAIT
    
    spmov = (spmov + 1) MOD 30
    SELECT CASE spmov/10
    CASE 0
        SPRITE SHOW 1, sp1
    CASE 1
        SPRITE SHOW 1, sp2
    CASE 2
        SPRITE SHOW 1, sp3
    CASE 3
        SPRITE SHOW 1, sp4
    END SELECT
    
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

