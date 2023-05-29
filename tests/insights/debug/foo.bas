OPTION EXPLICIT

'CLASS myc2
'    field1 AS INTEGER
'END CLASS

CLASS myc

    field1 AS INTEGER
    'field2 AS myc PTR

    'DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    'DECLARE FUNCTION Square() AS INTEGER

END CLASS

DIM SHARED AS INTEGER a, b

DIM SHARED AS myc PTR p1

p1 = NEW myc

TRACE "Calling the garbage collector..."

GC RUN

TRACE "GC finished."



