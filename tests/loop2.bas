' test EXIT

i% = 42

WHILE i% > 0
    i% = i% - 1

    IF i%<=32 THEN
        EXIT WHILE
    END IF
WEND

ASSERT i%=32

