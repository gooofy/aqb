'
' OOP Test 13
'
' multiple interfaces
'

OPTION EXPLICIT

INTERFACE i1

    DECLARE VIRTUAL SUB store (BYVAL i AS INTEGER)
    DECLARE VIRTUAL FUNCTION retrieve () AS INTEGER

END INTERFACE

INTERFACE i2

    DECLARE VIRTUAL FUNCTION square () AS INTEGER

END INTERFACE

CLASS myc1 IMPLEMENTS i1, i2

    field1 AS INTEGER

    DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    DECLARE VIRTUAL SUB store (BYVAL i AS INTEGER)
    DECLARE VIRTUAL FUNCTION retrieve () AS INTEGER

    DECLARE VIRTUAL FUNCTION square() AS INTEGER

END CLASS

CONSTRUCTOR myc1 (BYVAL initValue AS INTEGER)
    field1 = initValue
END CONSTRUCTOR

SUB myc1.store (BYVAL i AS INTEGER)
    field1 = i
END SUB

FUNCTION myc1.retrieve() AS INTEGER
    RETURN field1
END FUNCTION

FUNCTION myc1.square() AS INTEGER
    RETURN field1*field1
END FUNCTION

'
' main
'

' create object, test functionality via object ptr

DIM o AS myc1 PTR = NEW myc1(23)

DIM i AS INTEGER

i = o->retrieve()
'TRACE "i="; i
ASSERT i = 23

o->store(42)
i = o->retrieve()
'TRACE "i="; i
ASSERT i = 42

ASSERT o->square() = 1764

' now, convert o to interface ptr, test functionality by calling the intf procs

DIM iptr AS i1 PTR = o

i = iptr->retrieve()
'TRACE "i="; i
ASSERT i = 42

iptr->store(23)

i = iptr->retrieve()
'TRACE "i="; i
ASSERT i = 23

' test i2

DIM iptr2 AS i2 PTR = o

i = iptr2->square()
'TRACE "square result: "; i
ASSERT i=529

ASSERT o->retrieve()  = 23
'ASSERT o->retrieve2() = 42

'o->store(42)
'ASSERT o->retrieve() = 42
'ASSERT o->retrieve2() = 42
'
'o->store2(23)
'ASSERT o->retrieve() = 42
'ASSERT o->retrieve2() = 23
'ASSERT o->square() = 1764

