
FUNCTION fibonacci% ( a% )

    IF a%=1 OR a%=2 THEN
        fibonacci% = 1
    ELSE
        fibonacci% = fibonacci%(a%-1) + fibonacci%(a%-2)
    END IF

END FUNCTION

PRINT "Start..."

startTime = TIMER
print startTime

PRINT fibonacci%(23)

endTime = TIMER
print endTime

PRINT "Took:"; endTime-startTime; "s"


