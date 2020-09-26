'
' byref / byval tests
'
OPTION EXPLICIT

SUB sref (BYREF i AS INTEGER)
    i = i + 1
END SUB

SUB sval (BYVAL i AS INTEGER)
    i = i + 1
END SUB

DIM j AS INTEGER = 0

sref j
ASSERT j=1

sref j
ASSERT j=2

sval (j)
ASSERT j=2

sref j+1
ASSERT j=2

sref (j)
ASSERT j=2

