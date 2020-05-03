
SUB check1 ( b&, msg$ )

    IF b& <> 1 THEN
        PRINT msg$
        ASSERT FALSE
    END IF

END SUB

check1 1, "This shouldn't fail."

