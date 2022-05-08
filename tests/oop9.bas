'
' OOP Test 9
'
' constructor with all-optional arguments
'

OPTION EXPLICIT

TYPE myc1

    field1 AS INTEGER

    DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER = 42)

    DECLARE SUB store (BYVAL i AS INTEGER)
    DECLARE FUNCTION retrieve () AS INTEGER

END TYPE

CONSTRUCTOR myc1 (BYVAL initValue AS INTEGER)
    ' TRACE "CONSTRUCTOR myc1 called, this="; @this; ", initValue="; initValue
    field1 = initValue
END CONSTRUCTOR

SUB myc1.store (BYVAL i AS INTEGER)
    field1 = i
END SUB

FUNCTION myc1.retrieve () AS INTEGER
    ' TRACE "myc1.retrieve: field1=";field1
    RETURN field1
END FUNCTION

'
' main
'

DIM o AS myc1 PTR = NEW myc1()

' TRACE o->retrieve()

ASSERT o->retrieve()  = 42

o->store(23)
ASSERT o->retrieve() = 23

