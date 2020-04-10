
SUB myassert ( b&, msg$ )

    IF b& = 0 THEN
        PRINT "*** ASSERTION FAILED:", msg$
    END IF

END SUB

myassert 1=1, "This shouldn't fail."
myassert 1=0, "This should fail."

