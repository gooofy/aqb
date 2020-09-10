'
' OOP Test 4
'
' call methods without using this->
'

OPTION EXPLICIT

TYPE myc1

    field1 AS INTEGER

    DECLARE CONSTRUCTOR (initValue AS INTEGER)

    DECLARE SUB store (i AS INTEGER)
    DECLARE FUNCTION retrieve () AS INTEGER

    DECLARE FUNCTION square() AS INTEGER

END TYPE

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

DIM o AS myc1 = myc1(10)

o.store(42)
DIM res AS INTEGER = o.retrieve()
' _debug_puts "retrieved:" : _debug_puts2 res : _debug_putnl

ASSERT o.retrieve() = 42
o.store(23)
ASSERT o.retrieve() = 23
ASSERT o.square() = 529
