'
' test exit sub statement
'

OPTION EXPLICIT

DIM SHARED s AS INTEGER = 0

SUB foo (x AS INTEGER)

    FOR i AS INTEGER = 1 TO x

        ' PRINT "i:";i,x
        ' PRINT i

        IF i>5 THEN
            EXIT SUB
        END IF

        s = s + i

    NEXT i

END SUB

foo (1)
foo (2)
foo (11)
foo (12)

ASSERT s=34

