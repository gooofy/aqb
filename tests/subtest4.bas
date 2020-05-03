
REM test STATIC subprograms

FUNCTION counter%() STATIC

    cnt% = cnt% + 1
    counter%=cnt%

END FUNCTION

ASSERT(counter%()=1)
ASSERT(counter%()=2)
ASSERT(counter%()=3)
ASSERT(counter%()=4)

