'
' GC test 3: test gc auto-run
'

OPTION EXPLICIT

' ANY is not traced so we can use it as a weak pointer for test assertions
DIM SHARED AS ANY wp

CLASS myc2
    field1 AS INTEGER
END CLASS

CLASS myc

    field1 AS INTEGER
    field2 AS myc2 PTR

    'DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    'DECLARE FUNCTION Square() AS INTEGER

END CLASS

DIM SHARED AS myc PTR p

FOR i AS INTEGER = 1 TO 1000

    'TRACE i
    wp = p
    p = NEW myc

NEXT i

TRACE "main: Calling the garbage collector..."
GC_RUN
TRACE "main: GC finished."

ASSERT NOT GC_REACHABLE(wp)
ASSERT GC_REACHABLE(p)
'ASSERT NOT GC_REACHABLE(wp6)

