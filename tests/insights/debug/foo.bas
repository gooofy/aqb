OPTION EXPLICIT

'DIM b0
'DIM b1
DIM AS BOOLEAN b0, b1, b2
TRACE b0, b1, b2
PRINT b0, b1, b2
TRACE STR$(b0);STR$(b1);STR$(b2)
TRACE STR$(b0)+STR$(b1)+STR$(b2)
DIM s AS string = STR$(b0)+STR$(b1)+STR$(b2)
TRACE "s1: "; s
s = STR$(b2)
s = s + STR$(b1)
s = s + STR$(b0)
TRACE "s2: "; s

DIM b4 AS Boolean
DIM b5 AS Boolean
TRACE b4, b5

