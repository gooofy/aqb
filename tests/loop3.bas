' test EXIT in nested loops

OPTION EXPLICIT

DIM AS INTEGER i, j


FOR i = 10 TO 100

    j = 1
    WHILE j < 100

		' _debug_puts2(i) : _debug_puts(",") : _debug_puts2(j)
        j = j * 2

        IF i = j THEN
            EXIT WHILE, FOR
        END IF
    WEND
NEXT i

' _debug_puts2(i) : _debug_puts(",") : _debug_puts2(j)

ASSERT i=16
ASSERT j=16

