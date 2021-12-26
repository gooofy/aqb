REM
REM Load BOBs from an IFF ILBM file, animate them
REM

OPTION EXPLICIT
IMPORT IFFSupport
IMPORT AnimSupport

REM create a HIRES screen, put a backdrop window on it

SCREEN 1, 640, 200, 3, AS_MODE_HIRES, "BOB Demo"
WINDOW 1, ,(0,0)-(640,200), _
AW_FLAG_BACKDROP OR AW_FLAG_SIMPLE_REFRESH OR AW_FLAG_BORDERLESS OR AW_FLAG_ACTIVATE, 1

GELS INIT

REM load BOBs from IFF ILBM file

DIM AS BITMAP_t PTR gorillaBM = NULL
DIM AS BOB_t PTR gorilla(2)

ILBM LOAD BITMAP "PROGDIR:imgs/gorilla.iff", gorillaBM, 1

FOR i AS INTEGER = 0 TO 2
    gorilla(i) = BOB (gorillaBM, (i*32,0) - (i*32+31, 31))
NEXT i    

REM draw something on the background

COLOR 6

FOR x AS INTEGER = 10 TO 630 STEP 3
    LINE (x, 10)-(320,190)
NEXT x    

COLOR 5

LOCATE 12, 26
PRINT "Press mouse button to quit"

REM main loop: bob animation, event handling happens during VWAIT

SUB doQuit
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

DIM AS INTEGER vx=1, vy=1
DIM AS INTEGER x=100, y=100

DIM AS INTEGER curBOB=0

BOB SHOW gorilla(curBOB)

WHILE TRUE
    VWAIT
    BOB MOVE gorilla(curBOB), (x, y)
    
    x = x + vx
    y = y + vy
    
    IF x MOD 10 = 0 THEN
        
        BOB HIDE gorilla(curBOB)        
        curBOB = (curBOB+1) MOD 3
        BOB MOVE gorilla(curBOB), (x, y)        
        BOB SHOW gorilla(curBOB)        
        
    END IF        
    
    IF (x>500) OR (x<10) THEN
        vx = -vx        
    END IF
    
    IF (y>170) OR (y<20) THEN
        vy = -vy
    END IF
    
    GELS REPAINT    
WEND

