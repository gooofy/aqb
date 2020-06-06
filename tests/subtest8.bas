'
' amiga / ms basic syntax: _COORD2

' used in:
' GET (x1,y1)-(x2,y2), array-name [(index[, index...])]
' OBJECT.CLIP (x1,y1)-(x2,y2)
' SCROLL (x1,y1)-(x2,y2), delta-x, delta-y
' LINE [[STEP] (x1,y1)] - [STEP] (x2,y2), [colour-id][,b[f]]

DIM SHARED AS INTEGER sum

SUB WTEST (id AS INTEGER, _COORD2(s1 AS BOOLEAN=FALSE, _
                                  x1 AS INTEGER=-1,    _
                                  y1 AS INTEGER=-1,    _
                                  s2 AS BOOLEAN=FALSE, _
                                  x2 AS INTEGER=-1,    _
                                  y2 AS INTEGER=-1))

    ' PRINT s1, s2

    sum = id + x1 + y1 + x2 + y2 + s1 + s2

END SUB

sum = 0
WTEST 1
ASSERT sum = -3

sum = 0
WTEST 2, (0,0) - (100, 100)
ASSERT sum = 202

sum = 0
WTEST 2, STEP (0,0) - (100, 100)
ASSERT sum = 201

sum = 0
WTEST 2, (0,0) - STEP (100, 100)
ASSERT sum = 201

sum = 0
WTEST 2, - STEP (100, 100)
ASSERT sum = 199

sum = 0
WTEST 2, - (100, 100)
ASSERT sum = 200

