' test basic DO LOOP

OPTION EXPLICIT

DIM s AS INTEGER = 0

DO

    s = s + 1

    IF s > 41 THEN
        EXIT DO
    END IF

LOOP

ASSERT s = 42

