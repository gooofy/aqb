REM
REM Load a BOB from an IFF ILBM file
REM

OPTION EXPLICIT
IMPORT IFFSupport

SCREEN 1, 640, 200, 3, AS_MODE_HIRES, "BOB Demo"
WINDOW 1, ,(0,0)-(640,200), _
AW_FLAG_BACKDROP OR AW_FLAG_SIMPLE_REFRESH OR AW_FLAG_BORDERLESS OR AW_FLAG_ACTIVATE, 1

color 2

DIM AS BITMAP_t PTR gorilla = NULL

ILBM LOAD BITMAP "export:gorilla_32bb0.iff", gorilla, 1

DIM AS BOB_t PTR gbob

gbob = BOB (gorilla)

LOCATE 3,1 : PRINT "bob allocated, sleeping for 1s..."

SLEEP FOR 1

' BOB ON gbob, (100, 100)

FOR i AS INTEGER = 1 TO 20
    
    DIM AS INTEGER x= 100+i*2, y=100+i*2
    
    REM PRINT "moving bob to ";x;"/";y 
    
    REM SLEEP FOR 1    
    
    BOB MOVE gbob, (100+i*2, 100 + i*2)
    
    REM SLEEP FOR 1
    
NEXT i


PRINT "Press mouse button to quit"

SUB doQuit
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

WHILE TRUE
    SLEEP
WEND

