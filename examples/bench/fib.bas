
FUNCTION fibonacci% ( BYVAL a% )
    
    IF a%=1 OR a%=2 THEN
        fibonacci% = 1
    ELSE
        fibonacci% = fibonacci%(a%-1) + fibonacci%(a%-2)
    END IF
    
END FUNCTION

PRINT "Start..."

startTime = TIMER()

PRINT fibonacci%(23)

endTime = TIMER()

PRINT "Took:"; endTime-startTime; "s"

PRINT
PRINT "PRESS ENTER TO QUIT"

WHILE INKEY$ = ""
    SLEEP    
WEND


