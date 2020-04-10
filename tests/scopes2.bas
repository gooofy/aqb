DIM SHARED i

i = 42

SUB foo
    i = 23
    ASSERT i=23
END SUB

foo

ASSERT i=23

