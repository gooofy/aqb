REM c't HL Benchmarks

SUB RealMath STATIC

    startTime = TIMER()

    PRINT "Start"
    x = 0.0
    y = 9.9
    FOR i% = 1 TO 10000
        x = x + ( y * y - y ) / y
    NEXT i%
    PRINT "Finish :"; x

    stopTime = TIMER()

    PRINT "Took:";stopTime-startTime;"s"
END SUB

CALL RealMath

PRINT
PRINT "PRESS ENTER TO QUIT"

WHILE INKEY$ = ""
    SLEEP    
WEND

