'
' OOP Test 8
'
' PROPERTIES
'

OPTION EXPLICIT

CLASS myc1

    field1 AS INTEGER

    DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    DECLARE PROPERTY p1 AS INTEGER
    DECLARE PROPERTY p1 (BYVAL f AS INTEGER)

END CLASS

CONSTRUCTOR myc1 (BYVAL initValue AS INTEGER)
    ' TRACE "CONSTRUCTOR myc1 called, initValue="; initValue
    field1 = initValue
END CONSTRUCTOR

PROPERTY myc1.p1 AS INTEGER
    return field1
END PROPERTY

PROPERTY myc1.p1 (BYVAL f AS INTEGER)
    field1 = f
END PROPERTY

'
' main
'

DIM o AS myc1 PTR = NEW myc1(23)

' test getter
ASSERT o->p1 = 23

o->p1 = 42
ASSERT o->p1 = 42

