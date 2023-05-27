OPTION EXPLICIT

CLASS myc

    field1 AS INTEGER

    'DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    'DECLARE FUNCTION Square() AS INTEGER

END CLASS

DIM SHARED AS INTEGER a, b

DIM SHARED AS myc PTR p1

p1 = NEW myc

TRACE "Calling the garbage collector..."

GC RUN

TRACE "GC finished."



