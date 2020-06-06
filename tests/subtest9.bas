'
' amiga / ms basic syntax style function calls: _COORD
'

' used in:

' Area [STEP] (x, y)
' CIRCLE [STEP] (x, y) ,radius [,colour-id [,start, end [,aspect]]]
' PAINT [STEP] (x,y) [,paintColour-id] [,borderColour-id]
' POINT (x,y)
' PRESET [STEP] (x,y) [,colour-id]
' PSET [STEP] (x,y) [,colour-id]

DIM SHARED AS INTEGER sum

SUB PTEST (id AS INTEGER, _COORD(s1 AS BOOLEAN=FALSE, _
                                 x1 AS INTEGER=-1,    _
                                 y1 AS INTEGER=-1))

    ' PRINT s1, s2

    sum = id + x1 + y1 + s1

END SUB

sum = 0
PTEST 1
' PRINT sum
ASSERT sum = -1

sum = 0
PTEST 2, (23, 42)
' PRINT sum
ASSERT sum = 67

sum = 0
PTEST 2, STEP (23, 42)
' PRINT sum
ASSERT sum = 66

