' test short circuit evaluation

FUNCTION should%()
    PRINT "this should be executed."
    should% = -1
END FUNCTION

FUNCTION shouldnt%()
    PRINT "*** ERROR: this shouldn't be executed."
    shouldnt% = 0
END FUNCTION

i& = 42

IF ( i& > 23 ) AND should%() THEN
    PRINT "BOOL OK"
ELSE
    PRINT "*** BOOL FAILED"
END IF

IF ( i& < 23 ) AND shouldnt%() THEN
    PRINT "*** BOOL FAILED"
ELSE
    PRINT "BOOL OK"
END IF

IF ( i& > 23 ) OR shouldnt%() THEN
    PRINT "BOOL OK"
ELSE
    PRINT "*** BOOL FAILED"
END IF

IF ( i& < 23 ) OR should%() THEN
    PRINT "BOOL OK"
ELSE
    PRINT "*** BOOL FAILED"
END IF

IF NOT (i& < 23) THEN
    PRINT "BOOL OK"
ELSE
    PRINT "*** BOOL FAILED"
END IF

