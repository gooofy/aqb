'
' whole array assignment on heap test
'

DIM a%(100)
DIM b%(100)

FOR i% = 0 to 99
  a%(i%) = i%
  b%(i%) = i% * i%
NEXT i%

FOR i% = 0 to 99
  ASSERT a%(i%) = i%
NEXT i%

a% = b%

FOR i% = 0 to 99
  ASSERT a%(i%) = i%*i%
NEXT i%

