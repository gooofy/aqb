'
' GC test 2: find roots in stack frames
'

OPTION EXPLICIT

' ANY is not traced so we can use it as a weak pointer for test assertions
DIM SHARED AS ANY wp4, wp5, wp6

CLASS myc2
    field1 AS INTEGER
END CLASS

CLASS myc

    field1 AS INTEGER
    field2 AS myc2 PTR

    'DECLARE CONSTRUCTOR (BYVAL initValue AS INTEGER)

    'DECLARE FUNCTION Square() AS INTEGER

END CLASS

SUB foo

    DIM AS myc PTR p4
    DIM AS myc PTR p5

    p4 = NEW myc
    p5 = NEW myc

    p4->field2 = NEW myc2

    wp4 = p4
    wp5 = p5
    wp6 = p4->field2

    ASSERT GC_REACHABLE(wp4)
    ASSERT GC_REACHABLE(wp5)
    ASSERT GC_REACHABLE(wp6)

    TRACE "foo: Calling the garbage collector..."
    GC_RUN
    TRACE "foo: GC finished."

    ASSERT GC_REACHABLE(wp4)
    ASSERT GC_REACHABLE(wp5)
    ASSERT GC_REACHABLE(wp6)

    'TRACE "foo: Calling the garbage collector..."
    'GC_RUN
    'TRACE "foo: GC finished."

    TRACE "foo: p4=NULL"
    p4 = NULL

    TRACE "foo: Calling the garbage collector..."
    GC_RUN
    TRACE "foo: GC finished."

    ASSERT NOT GC_REACHABLE(wp4)
    ASSERT GC_REACHABLE(wp5)
    ASSERT NOT GC_REACHABLE(wp6)

END SUB

foo

TRACE "main: foo returned."

TRACE "main: Calling the garbage collector..."
GC_RUN
TRACE "main: GC finished."

ASSERT NOT GC_REACHABLE(wp4)
ASSERT NOT GC_REACHABLE(wp5)
ASSERT NOT GC_REACHABLE(wp6)


'DIM SHARED AS INTEGER a, b
'
'DIM SHARED AS myc PTR p1, p2, p3
'
'' ANY is not traced so we can use it as a weak pointer for test assertions
'DIM AS ANY wp1, wp2, wp3
'
'p1 = NEW myc
'p2 = p1
'p3 = NEW myc
'
'wp1 = p1
'wp2 = p3
'
'p1->field2 = NEW myc2
'wp3 = p1->field2
'
'TRACE "Calling the garbage collector..."
'GC_RUN
'TRACE "GC finished."
'
'ASSERT GC_REACHABLE(wp1)
'ASSERT GC_REACHABLE(wp2)
'ASSERT GC_REACHABLE(wp3)
'
'TRACE "p1 = NULL"
'p1 = NULL
'
'TRACE "Calling the garbage collector..."
'GC_RUN
'TRACE "GC finished."
'
'TRACE "p2 = NULL"
'p2 = NULL
'
'TRACE "Calling the garbage collector..."
'GC_RUN
'TRACE "GC finished."
'
'ASSERT NOT GC_REACHABLE(wp1)
'ASSERT GC_REACHABLE(wp2)
'ASSERT NOT GC_REACHABLE(wp3)
'
'TRACE "p3 = NULL"
'p3 = NULL
'
'TRACE "Calling the garbage collector..."
'GC_RUN
'TRACE "GC finished."
'
'ASSERT NOT GC_REACHABLE(wp1)
'ASSERT NOT GC_REACHABLE(wp2)
'ASSERT NOT GC_REACHABLE(wp3)

