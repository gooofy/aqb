'
' test allocate/deallocate
'

OPTION EXPLICIT

DIM SHARED AS ANY PTR wp1, wp2, wp3

wp1 = ALLOCATE (512)
wp2 = ALLOCATE (512)
wp3 = ALLOCATE (512)

DEALLOCATE (wp2)
DEALLOCATE (wp1)


