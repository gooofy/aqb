'
' OOP Test 1
'

OPTION EXPLICIT

TYPE myc1

    field1 AS INTEGER

    DECLARE SUB store (o AS INTEGER)
    DECLARE FUNCTION retrieve () AS INTEGER

END TYPE

SUB myc1.store (i AS INTEGER)

    ' _debug_puts "store() called." : _debug_putnl

    this->field1 = i

END SUB

FUNCTION myc1.retrieve () AS VOID PTR

    ' _debug_puts "retrieve() called." : _debug_putnl

    RETURN this->field1

END FUNCTION

'
' main
'

DIM o AS myc1

o.store(42)
DIM res AS INTEGER = o.retrieve()
' _debug_puts "retrieved:" : _debug_puts2 res : _debug_putnl

ASSERT o.retrieve() = 42
o.store(23)
ASSERT o.retrieve() = 23


