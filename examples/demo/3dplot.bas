'
' 3D Function Plotter Demo
'

OPTION EXPLICIT

IF FRE(-2) < 20000 THEN
    PRINT "*** Error: stack size too small (need at least 20KBytes)"
    ERROR 42
END IF

SCREEN 2, 640, 200, 2, AS_MODE_HIRES, "3D Function Plot"

WINDOW 4,,,AW_FLAG_BACKDROP OR AW_FLAG_BORDERLESS,2

DIM AS INTEGER wx=WINDOW(2), wy=WINDOW(3)-10

PALETTE 0,  0,  0,  0
PALETTE 1,  1,  1,  1 : REM text color white
PALETTE 2, .1, .9, .9 : REM net color light blue
PALETTE 3, .2, .2,  1 : REM area color blue

FUNCTION FNfun(BYVAL x AS SINGLE, BYVAL z AS SINGLE) AS SINGLE
    RETURN 20*SIN(.1*SQR(x*x+z*z))
END FUNCTION

' 3D projection config

CONST AS SINGLE tx=0, ty=0, tz=0                         : REM translation
CONST AS SINGLE sx=3, sy=3, sz=3                         : REM scaling
CONST AS SINGLE rx=30*PI/180, ry=-20*PI/180, rz=0*PI/180 : REM rotation

CONST AS SINGLE camera_x=0, camera_y=0, camera_z=-400

' 2D
DIM   AS SINGLE trans_x=wx/2, trans_y=wy/2
CONST AS SINGLE scale_x=1, scale_y=2

' plot range etc

CONST AS SINGLE xa=-65, xb=65 : REM X interval
CONST AS SINGLE ya=-30, yb=60 : REM Y interval
CONST AS SINGLE za=-60, zb=60 : REM Z interval
CONST AS SINGLE prec=30       : REM precision (number of values to compute per X interval)
CONST AS SINGLE net_x=15      : REM network depth (# lines per X-interval)
CONST AS SINGLE net_z=30      : REM network depth (# lines per Z-interval)

' precompute 3d projection matrix

DIM AS SINGLE A = COS(ry)*COS(rz)
DIM AS SINGLE B = COS(ry)*SIN(rz)
DIM AS SINGLE C = -SIN(ry)
DIM AS SINGLE D = SIN(rx)*SIN(ry)*COS(rz) - COS(rx)*SIN(rz)
DIM AS SINGLE E = SIN(rx)*SIN(ry)*SIN(rz) + COS(rx)*COS(rz)
DIM AS SINGLE F = SIN(rx)*COS(ry)
DIM AS SINGLE G = COS(rx)*SIN(ry)*COS(rz) + SIN(rx)*SIN(rz)
DIM AS SINGLE H = COS(rx)*SIN(ry)*SIN(rz) - SIN(rx)*COS(rz)
DIM AS SINGLE I = COS(rx)*COS(ry)

' compute steps
CONST AS SINGLE  ste_z      = (zb-za)/(net_z-1)
CONST AS SINGLE  ste_x      = (xb-xa)/(prec-1)
CONST AS INTEGER num_step_x = prec/net_x

' background pattern (just to test the PATTERN command)
DIM AS INTEGER pattern_bg(3)

pattern_bg(0) = &HCCCC
pattern_bg(1) = &H3333
pattern_bg(2) = &HCCCC
pattern_bg(3) = &H3333

PATTERN ,pattern_bg
LINE (0,0)-(wx,wy),1,BF

' function pattern
DIM AS INTEGER pattern_fg(0)

pattern_fg(0) = &HFFFF

PATTERN ,pattern_fg

' cache up to two rows of f() values
DIM AS INTEGER  fncache_x(STATIC prec-1, 1), fncache_y(STATIC prec-1, 1)
DIM AS INTEGER  fncache_cur = 0, fncache_last = 1

' main drawing routine

DIM AS SINGLE  x, y, z : REM transform input
DIM AS INTEGER x1, y1  : REM transform result -> draw

DIM AS INTEGER net_cnt
DIM AS INTEGER fncache_cnt

AREA OUTLINE TRUE
COLOR 3,,2

FOR z = zb TO za STEP -ste_z

    ' FIXME: SWAP fn_cache_cur, fn_cache_last

    DIM tmp AS INTEGER = fncache_cur
    fncache_cur = fncache_last
    fncache_last = tmp

    net_cnt     = -1
    fncache_cnt =  0

    FOR x = xa TO xb STEP ste_x

        ' PRINT x, z

        y = FNfun(x, z)

        GOSUB transform
        GOSUB draw

    NEXT x

NEXT z

LOCATE 23,0
COLOR 1
PRINT "PRESS ANY KEY TO QUIT"

WHILE INKEY$()=""
WEND

END

'
' subroutines (not using SUBs/FUNCTIONs here as this serves as a GOSUB+RETURN test)

' 3D (x, y, z) -> 2D (x1, y1) coord transformation
transform:

    ' scale and translate

    DIM AS SINGLE xt1 = sx*x + tx
    DIM AS SINGLE yt1 = sy*y + ty
    DIM AS SINGLE zt1 = sz*z + tz

    ' rotate

    DIM AS SINGLE xt2 = xt1*A + yt1*B + zt1*C
    DIM AS SINGLE yt2 = xt1*D + yt1*E + zt1*F
    DIM AS SINGLE zt2 = xt1*G + yt1*H + zt1*I

    ' project

    DIM AS SINGLE tmp = zt2 - camera_z
    x1 = camera_x - camera_z * (xt2-camera_x)/tmp
    y1 = camera_y - camera_z * (yt2-camera_y)/tmp

    ' 2D translation and scaling
    x1 = trans_x + x1/scale_x
    y1 = trans_y - y1/scale_y

    ' simple clipping
    IF x1<0  THEN x1=0
    IF x1>wx THEN x1=wx
    IF y1<0  THEN y1=0
    IF y1>wy THEN y1=wy

RETURN

' store x1/y1 coords in fncache, draw polygon
draw:

    ' print fncache_cnt, fncache_cur

    fncache_x(fncache_cnt, fncache_cur) = x1
    fncache_y(fncache_cnt, fncache_cur) = y1

    net_cnt = net_cnt + 1

    ' time to draw a polygon?
    IF net_cnt=num_step_x AND z<>zb AND y>=ya AND y<=yb THEN

        net_cnt = 0

        ' initialize AREA polynom
        FOR ind AS INTEGER = fncache_cnt-num_step_x TO fncache_cnt
            x1 = fncache_x(ind, fncache_cur)
            y1 = fncache_y(ind, fncache_cur)
            AREA (x1, y1)
        NEXT ind
        FOR ind AS INTEGER = fncache_cnt TO fncache_cnt-num_step_x STEP -1
            x1 = fncache_x(ind, fncache_last)
            y1 = fncache_y(ind, fncache_last)
            AREA (x1, y1)
        NEXT ind

        AREAFILL 0

    END IF

    fncache_cnt = fncache_cnt + 1  : REM next point

RETURN

