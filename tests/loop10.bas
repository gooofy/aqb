' test DO LOOP UNTIL

OPTION EXPLICIT

DIM s AS INTEGER = 0

DO

    s = s + 1

LOOP UNTIL s=42

' PRINT s

ASSERT s = 42

