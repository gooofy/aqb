'
' dynamic array allocation and access test
'

OPTION EXPLICIT

' DECLARE FUNCTION ALLOCATE (size AS ULONG, flags AS ULONG=0) AS VOID PTR

CONST num AS INTEGER = 10

DIM a AS UINTEGER PTR
DIM i AS INTEGER

a = ALLOCATE ( num * SIZEOF (UINTEGER) )

FOR i = 0 TO num-1
    a[i] = i
NEXT i

DIM s AS INTEGER = 0
FOR i = 0 TO num-1
    s = s + a[i]
NEXT i

' PRINT s
ASSERT s = 45

