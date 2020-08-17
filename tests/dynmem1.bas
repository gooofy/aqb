'
' dynamic array allocation and access test
'

OPTION EXPLICIT

' DECLARE FUNCTION ALLOCATE (size AS ULONG, flags AS ULONG=0) AS VOID PTR

CONST num AS INTEGER = 10

DIM a AS UINTEGER PTR
DIM i AS INTEGER

' _debug_puts2 ( num * SIZEOF (UINTEGER) )
a = ALLOCATE ( num * SIZEOF (UINTEGER) )

FOR i = 0 TO num-1
	' _debug_puts2 i
	' _debug_puts "::"
    a[i] = i
NEXT i

DIM s AS INTEGER = 0
FOR i = 0 TO num-1
    s = s + a[i]
NEXT i

' _debug_puts2 s
ASSERT s = 45

