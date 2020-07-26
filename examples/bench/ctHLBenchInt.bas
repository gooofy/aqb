
REM c't HL Benchmarks

SUB IntMath STATIC

  startTime = TIMER()

  PRINT "Start"
  x& = 0
  y& = 3
  FOR i% = 1 TO 10000
    x& = x& + ( y&*y&-y& ) / y&
  NEXT i%
  PRINT "Finish :", x&

  stopTime = TIMER()

  PRINT "Took:"; stopTime-startTime; "s"

END SUB

CALL IntMath


