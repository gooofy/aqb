' test for CONTINUE

OPTION EXPLICIT

DIM AS INTEGER i, s=0, l=0

FOR i = 10 TO 100

    l = l + 1

    IF i < 50 THEN
        CONTINUE
    END IF

    s = s + 1
NEXT i

' PRINT l, s

ASSERT l=91
ASSERT s=51

