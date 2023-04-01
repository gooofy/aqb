'
' OOP Test 5
'
' new operator
'

OPTION EXPLICIT

CLASS myc1

    field1 AS INTEGER

    DECLARE CONSTRUCTOR (initValue AS INTEGER)

    DECLARE SUB store (i AS INTEGER)
    DECLARE FUNCTION retrieve () AS INTEGER

    DECLARE FUNCTION square() AS INTEGER

END CLASS

CONSTRUCTOR myc1 (initValue AS INTEGER)
    ' _debug_puts "CONSTRUCTOR called." : _debug_putnl
    field1 = initValue
END CONSTRUCTOR

SUB myc1.store (i AS INTEGER)

    ' _debug_puts "store() called." : _debug_putnl

    field1 = i

END SUB

FUNCTION myc1.square () AS INTEGER
    RETURN retrieve()*retrieve()
END FUNCTION

FUNCTION myc1.retrieve () AS INTEGER

    ' _debug_puts "retrieve() called." : _debug_putnl

    RETURN field1

END FUNCTION

'
' main
'

' TRACE "1"

DIM o AS myc1 PTR = NEW myc1(10)

' TRACE "2"
o->store(42)

' TRACE "3"
ASSERT o->retrieve() = 42
'TRACE "4"
o->store(23)
'TRACE "5"
ASSERT o->retrieve() = 23
'TRACE "6"
ASSERT o->square() = 529

