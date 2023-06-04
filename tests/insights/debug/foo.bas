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

TRACE "main starts..."

DIM SHARED AS INTEGER a, b

DIM SHARED AS myc PTR p1, p2, p3

p1 = NEW myc
p2 = p1
p3 = NEW myc

TRACE "Calling the garbage collector..."
GC_RUN
TRACE "GC finished."

TRACE "p1 = NULL"

p1 = NULL

TRACE "Calling the garbage collector..."
GC_RUN
TRACE "GC finished."

TRACE "p1 = NULL"

p2 = NULL

TRACE "Calling the garbage collector..."
GC_RUN
TRACE "GC finished."

