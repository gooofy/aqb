REM
REM Sprite animation, extract multiple frames from one IFF
REM

OPTION EXPLICIT
IMPORT AnimSupport

WINDOW 1, "SPRITE Demo 3 - Animation"

DIM AS BITMAP_t PTR bananaBM = NULL

ILBM LOAD BITMAP "PROGDIR:imgs/banana.iff", bananaBM

DIM AS SPRITE_t PTR sp(23)

FOR i AS INTEGER = 0 TO 11
    sp(i) = SPRITE (bananaBM, (i*16,0)-(i*16+15,15))
    sp(i+12) = SPRITE (bananaBM, (i*16,16)-(i*16+15,31))    
NEXT i    

REM sprite colors

PALETTE 21, 0.8, 0.8, 0
PALETTE 22, 0, 0, 0
PALETTE 23, 1, 1, 0

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
    
    spmov = (spmov + 1) MOD 24
    SPRITE SHOW 2, sp(spmov)    
    
    
    SPRITE MOVE 2, (x, y)    
    
    x = x + vx
    y = y + vy
    
    IF (x>500) OR (x<10) THEN
        vx = -vx        
    END IF
    
    IF (y>170) OR (y<20) THEN
        vy = -vy
    END IF
    
WEND

