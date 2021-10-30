REM
REM Draw into an offscreen bitmap
REM

OPTION EXPLICIT

WINDOW 1, "Offscreen bitmap draw demo"

REM create an offscreen bitmap

DIM AS BITMAP_t PTR bm = BITMAP(32, 32, 2)

REM draw some graphics and text into it

BITMAP OUTPUT bm
LINE (0,0)-(31,31),2,B
LINE (1,1)-(30,30),3,BF
COLOR 1,,,DRMD_JAM1 : LOCATE XY (8,18) : PRINT "BM"

REM switch output back to our window

WINDOW OUTPUT 1

REM display our bitmap multiple times

FOR x AS INTEGER = 20 TO 500 STEP 42
    PUT (x, 20), bm
NEXT x

REM finish

LOCATE 12, 26
PRINT "Press mouse button to quit"

SUB doQuit
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

WHILE TRUE
    SLEEP    
WEND

