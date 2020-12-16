'
' test GOSUB <-> FOR loop interaction
'

OPTION EXPLICIT

CONST AS INTEGER arr_sz_z = 101

DIM SHARED AS INTEGER x1

CONST AS SINGLE camera_z=-400

DIM AS INTEGER cnt=0

FOR iz AS INTEGER = 0 TO arr_sz_z - 1

    GOSUB transform

    cnt = cnt + 1

NEXT iz

' _debug_puts2 cnt : _debug_putnl

ASSERT cnt=101

END

transform:

    DIM AS SINGLE tmp2 = 42
    DIM AS SINGLE xt2 = 42

    x1 = camera_z * xt2/tmp2

RETURN

