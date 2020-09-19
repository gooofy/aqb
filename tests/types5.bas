
'
' string array-like access test
'

DIM pa AS UBYTE PTR

a$ = "ABCDE"

pa = a$

' read via pointer

s%=0
FOR i% = 0 TO 4
    s% = s% + pa[i%]
NEXT i%

' PRINT s%

ASSERT s%=335

' modify via pointer

pa[0] = 65
pa[1] = 65
pa[2] = 65
pa[3] = 65
pa[4] = 65

s%=0
FOR i% = 0 TO 4
    'PRINT i%, pa[i%]
    s% = s% + pa[i%]
NEXT i%

' PRINT a$, s%

ASSERT s%=325


