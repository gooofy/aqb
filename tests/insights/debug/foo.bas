'
' OOP Test 12
'
' interfaces
'

OPTION EXPLICIT

INTERFACE i1

    DECLARE VIRTUAL SUB store (BYVAL i AS INTEGER)
    DECLARE VIRTUAL FUNCTION retrieve () AS INTEGER

END INTERFACE

CLASS myc1 IMPLEMENTS i1

    field1 AS INTEGER

    DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    DECLARE VIRTUAL SUB store (BYVAL i AS INTEGER)
    DECLARE VIRTUAL FUNCTION retrieve () AS INTEGER

    'DECLARE FUNCTION square() AS INTEGER

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

'
' main
'

'TRACE "HUBBA"

DIM o AS myc1 PTR = NEW myc1(23)

'ASSERT o->retrieve()  = 23
'ASSERT o->retrieve2() = 42

'o->store(42)
'ASSERT o->retrieve() = 42
'ASSERT o->retrieve2() = 42
'
'o->store2(23)
'ASSERT o->retrieve() = 42
'ASSERT o->retrieve2() = 23
'ASSERT o->square() = 1764

