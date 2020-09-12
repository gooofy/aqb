
'
' pointer array-like access test
'

DIM a(STATIC 10) AS INTEGER
DIM pa AS INTEGER PTR

pa = @a[0]

' write via pointer, read via array

FOR i% = 0 TO 9
    'a[i%] = i%
    pa[i%] = i%
NEXT i%

s%=0
FOR i% = 0 TO 9
    s% = s% + a[i%]
NEXT i%

ASSERT s%=45

' write via array, read via pointer

FOR i% = 0 TO 9
    a[i%] = i%
NEXT i%

s%=0
FOR i% = 0 TO 9
    s% = s% + pa[i%]
NEXT i%

ASSERT s%=45


