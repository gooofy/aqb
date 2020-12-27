'
' excerpt from 3dplot demo that creates a bit of register pressure
'

OPTION EXPLICIT

DIM AS INTEGER wx=640, wy=200
DIM AS SINGLE trans_x=170, trans_y=80

DIM SHARED AS SINGLE  x, y, z : REM transform input
DIM SHARED AS INTEGER x1, y1  : REM transform result -> draw

CONST AS INTEGER arr_sz_x = 101
CONST AS INTEGER arr_sz_z = 101

' precompute all f() values
DIM AS INTEGER  fncache_x1(STATIC arr_sz_x, arr_sz_z)
DIM AS INTEGER  fncache_y1(STATIC arr_sz_x, arr_sz_z)
DIM AS SINGLE   fncache_y (STATIC arr_sz_x, arr_sz_z)

' z = zb
FOR iz AS INTEGER = 0 TO arr_sz_z - 1

    ' LOCATE 3,1 : PRINT "Precomputing fn values ";iz+1;"/";arr_sz_z;", please wait...   "
    ' x = xa

    FOR ix AS INTEGER = 0 TO arr_sz_x - 1

        ' LOCATE 4,1 : PRINT ix,"/",arr_sz_x-1

        fncache_y(ix, iz) = y

        GOSUB transform

        fncache_x1(ix, iz) = x1
        'fncache_y1(ix, iz) = y1

    NEXT ix

    ' z = z - ste_z

NEXT iz

END

'
' subroutines (not using SUBs/FUNCTIONs here as this serves as a GOSUB+RETURN test)
'

' 3D (x, y, z) -> 2D (x1, y1) coord transformation
transform:

    ' 2D translation and scaling
    x1 = trans_x + x1
    y1 = trans_y - y1

    ' simple clipping
    IF x1<0  THEN x1=0
    IF x1>wx THEN x1=wx
    IF y1<0  THEN y1=0
    IF y1>wy THEN y1=wy

    'PRINT "transform: ";x;y;z;" -> ";x1;y1
RETURN

