
SUB assert ( b&, msg$ )

    IF b& = 0 THEN
        PRINT "*** ASSERTION FAILED:", msg$
    END IF

END SUB

assert 1=1, "This shouldn't fail."
assert 1=0, "This should fail."

