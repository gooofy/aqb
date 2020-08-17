' test short circuit evaluation

DIM SHARED cnt AS INTEGER = 0

'FUNCTION should%()
'    cnt = cnt + 1
'    should% = -1
'END FUNCTION

FUNCTION shouldnt%()
    ASSERT FALSE
    shouldnt% = 0
END FUNCTION

i& = 42

'IF ( i& > 23 ) AND should%() THEN
'    ASSERT TRUE
'ELSE
'    ASSERT FALSE
'END IF

IF ( i& < 23 ) AND shouldnt%() THEN
    ASSERT FALSE
ELSE
    ASSERT TRUE
END IF

'IF ( i& > 23 ) OR shouldnt%() THEN
'    ASSERT TRUE
'ELSE
'    ASSERT FALSE
'END IF
'
'IF ( i& < 23 ) OR should%() THEN
'    ASSERT TRUE
'ELSE
'    ASSERT FALSE
'END IF
'
'IF NOT (i& < 23) THEN
'    ASSERT TRUE
'ELSE
'    ASSERT FALSE
'END IF
'
'ASSERT cnt=2


