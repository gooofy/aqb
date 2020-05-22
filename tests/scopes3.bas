' test local FOR loop var

OPTION EXPLICIT

DIM i AS INTEGER = 23

DIM j AS UINTEGER = 0

FOR i AS UINTEGER = 1 TO 10     : REM uses local i
    j = j + i
NEXT i

' PRINT i, j

ASSERT j = 55
ASSERT i = 23                   : REM global i should have remained intact


