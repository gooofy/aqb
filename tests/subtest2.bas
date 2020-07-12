
REM test STATIC subprogram vars

FUNCTION counter%()

    STATIC cnt%
    cnt% = cnt% + 1
    counter%=cnt%

END FUNCTION

FUNCTION counter2() AS INTEGER

    STATIC AS INTEGER cnt = 0
    cnt = cnt + 1

    ' PRINT cnt

    RETURN cnt

END FUNCTION

ASSERT(counter%()=1)
ASSERT(counter%()=2)
ASSERT(counter%()=3)
ASSERT(counter%()=4)


ASSERT(counter2()=1)
ASSERT(counter2()=2)
ASSERT(counter2()=3)
ASSERT(counter2()=4)

