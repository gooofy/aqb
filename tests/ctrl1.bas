' test ELSEIF

OPTION EXPLICIT

DIM AS INTEGER s = 0, r = 0, t = 0

FOR i AS INTEGER = 0 TO 99

    IF i<10 THEN
        'PRINT i; "<10"
        s = s + i
    ELSEIF i<50 THEN
        'PRINT "10<=";i; "<50"
        r = r + i
    ELSE
        'PRINT i; ">=10"
        t = t + i
    END IF

NEXT i

'PRINT s, r, t

ASSERT s=45
ASSERT r=1180
ASSERT t=3725


