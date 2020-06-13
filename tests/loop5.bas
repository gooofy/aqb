' test while CONTINUE

OPTION EXPLICIT

DIM AS INTEGER i=10, s=0, l=0

WHILE i<=100

    i = i + 1
    l = l + 1

    IF i < 50 THEN
        CONTINUE
    END IF

    s = s + 1
WEND

'PRINT l, s

ASSERT l=91
ASSERT s=52

