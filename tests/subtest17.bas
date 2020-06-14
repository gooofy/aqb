'
' test exit function statement
'

OPTION EXPLICIT

DIM r AS INTEGER = 0

FUNCTION foo (x AS INTEGER) AS INTEGER

    DIM s AS INTEGER = 0

    FOR i AS INTEGER = 1 TO x

        ' PRINT "i:";i,x,s
        ' PRINT i

        IF i>5 THEN
            foo = s
            EXIT FUNCTION
        END IF

        s = s + i

    NEXT i

    foo = s

END FUNCTION

r = r + foo (1)
r = r + foo (2)
r = r + foo (11)
r = r + foo (12)

ASSERT r=34

