REM
REM Load a BOB from an IFF ILBM file, animate it
REM

OPTION EXPLICIT
IMPORT IFFSupport
IMPORT AnimSupport

REM create a HIRES screen, put a backdrop window on it

SCREEN 1, 640, 200, 3, AS_MODE_HIRES, "BOB Demo"
WINDOW 1, ,(0,0)-(640,200), _
AW_FLAG_BACKDROP OR AW_FLAG_SIMPLE_REFRESH OR AW_FLAG_BORDERLESS OR AW_FLAG_ACTIVATE, 1

GELS INIT

REM load BOB from IFF file

DIM AS BOB_t PTR gorilla = NULL

ILBM LOAD BOB "PROGDIR:imgs/gorilla_32bb0.iff", gorilla, 1

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

BOB SHOW gorilla

WHILE TRUE
    VWAIT
    BOB MOVE gorilla, (x, y)
    
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

