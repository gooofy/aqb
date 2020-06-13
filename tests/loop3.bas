' test EXIT in nested loops

OPTION EXPLICIT

DIM AS INTEGER i, j


FOR i = 10 TO 100

    j = 1
    WHILE j < 100

        j = j * 2

        IF i = j THEN
            EXIT WHILE, FOR
        END IF
    WEND
NEXT i

ASSERT i=16
ASSERT j=16

