OPTION EXPLICIT

' --------------------------------------------------------------------------------------------------------
' --
' -- simple screen / window / graphics demo program
' --
' --------------------------------------------------------------------------------------------------------

DIM SHARED finished AS BOOLEAN = FALSE

SUB HandleWindow (BYVAL wid AS INTEGER)
    finished = TRUE
    PRINT "HandleWindow() called"
END SUB

WINDOW 1, "gfx1 - Simple AQB Graphics Demo", ( 0, 0 ) - ( 638, 180 )

ON WINDOW CLOSE CALL 1, HandleWindow

PRINT "Text output test"

FOR i AS INTEGER = 1 TO 76
    PRINT CHR$ ( i MOD 10 + 48 );
NEXT i
PRINT

FOR i AS INTEGER = 1 TO 10
    PRINT "."; CHR$ ( 9 );
NEXT i
PRINT

FOR i AS INTEGER = 1 TO 6
    PRINT STR$ ( i ),
NEXT i
PRINT

FOR c AS INTEGER = 0 TO 3
    
    LINE ( c * 60, 33 ) - ( c * 60 + 59, 48 ), 3 - c, BF
    LINE ( c * 60, 33 ) - ( c * 60 + 59, 48 ), c, B
    
NEXT c

FOR x AS INTEGER = 1 TO 600 STEP 5
    LINE ( x, 50 ) - ( 300, 140 ), 1
NEXT

LOCATE, 1 : PRINT "PRESS ANY KEY FOR SCROLL TEST";

WHILE INKEY$ ( ) = ""
    SLEEP
WEND

PRINT

' test WINDOW() function
FOR i AS INTEGER = 0 TO 13
    PRINT "WINDOW("; i; ") =", WINDOW ( i )
NEXT i

PRINT
PRINT "CLOSE WINDOW TO QUIT"

WHILE NOT finished
    SLEEP
WEND

' CloseWindow g_output_win

' WINDOW CLOSE 1 : REM: should happen automatically through exit handler now


