'
' SUB pointer assignment
'

OPTION EXPLICIT

DIM SHARED g AS INTEGER

DIM AS SUB (BYVAL INTEGER) s1

SUB mys (BYVAL i AS INTEGER)

    TRACE "mys called, i=";i

    g = i

END SUB

g = 23

s1 = mys

s1(42)

ASSERT g=42


