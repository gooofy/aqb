i& = 42

l&=0 : u&=0

FOR i& = 1 TO 42

    ' PRINT i&;

    IF i& > 23 THEN
        u& = u& + 1
        ' PRINT ">23", u&, l&
    ELSE
        l& = l& + 1
        ' PRINT "<=23", u&, l&
    END IF

NEXT i&

' PRINT u&, l&

ASSERT u&=19
ASSERT l&=23


