' test DO UNTIL LOOP

OPTION EXPLICIT

DIM s AS INTEGER = 0

DO UNTIL s=42

    s = s + 1

LOOP

' PRINT s

ASSERT s = 42

