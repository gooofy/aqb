'
' 3D Function Plotter Demo
'

OPTION EXPLICIT

IF FRE(-2) < 20000 THEN
    PRINT "*** Error: stack size too small (need at least 20KBytes)"
    ERROR 42
END IF

SCREEN 2, 640, 200, 3, AS_MODE_HIRES, "3D Function Plot"

WINDOW 4,,,AW_FLAG_BACKDROP OR AW_FLAG_BORDERLESS,2

DIM AS INTEGER wx=WINDOW(2), wy=WINDOW(3)-10

' OS 2.0 colors
PALETTE 0, 10.0/15.0, 10.0/15.0, 10.0/15.0 : REM gray
PALETTE 1,       0.0,       0.0,       0.0 : REM black
PALETTE 2,       1.0,       1.0,       1.0 : REM white

' function plot area shading colors
PALETTE 3, 0.0, 0.0, 0.8
PALETTE 4, 0.0, 0.8, 0.8
PALETTE 5, 0.0, 0.8, 0.0
PALETTE 6, 0.8, 0.8, 0.0
PALETTE 7, 0.8, 0.0, 0.0

' 3D projection config
CONST AS SINGLE tx=0, ty=0, tz=0                          : REM translation
CONST AS SINGLE sx=3, sy=3, sz=3                          : REM scaling
CONST AS SINGLE rx=30*PI/180, ry=-20*PI/180, rz= 0*PI/180 : REM rotation

CONST AS SINGLE camera_x=0, camera_y=0, camera_z=-400

' 2D
DIM   AS SINGLE trans_x=wx/2+10, trans_y=wy/2-20
CONST AS SINGLE scale_x=1, scale_y=2

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

DIM SHARED AS SINGLE  x, y, z : REM transform input
DIM SHARED AS INTEGER x1, y1  : REM transform result -> draw

' function to plot
FUNCTION FNfun(BYVAL x AS SINGLE, BYVAL z AS SINGLE) AS SINGLE
    RETURN 20*SIN(.1*SQR(x*x+z*z))
END FUNCTION

' plot range etc.
CONST AS SINGLE  xa=-65, xb=65 : REM X interval
CONST AS SINGLE  ya=-20, yb=20 : REM Y interval
CONST AS SINGLE  za=-60, zb=60 : REM Z interval
CONST AS INTEGER grid_cx=20    : REM grid x size (in tiles)
CONST AS INTEGER grid_cz=20    : REM grid z size (in tiles)
CONST AS INTEGER prec_x=5      : REM x precision (number of values to compute per grid tile)
CONST AS INTEGER prec_z=5      : REM x precision (number of values to compute per grid tile)

' compute steps
CONST AS INTEGER arr_sz_x = grid_cx * prec_x + 1
CONST AS INTEGER arr_sz_z = grid_cz * prec_z + 1
CONST AS SINGLE  ste_x    = (xb-xa)/(arr_sz_x - 1)
CONST AS SINGLE  ste_z    = (zb-za)/(arr_sz_z - 1)

' patterns (used for dithering)
DIM AS INTEGER pattern25(1)

pattern25(0) = &HEEEE
pattern25(1) = &H7777

DIM AS INTEGER pattern50(1)

pattern50(0) = &HCCCC
pattern50(1) = &H3333

DIM AS INTEGER pattern75(1)

pattern75(0) = &H1111
pattern75(1) = &H8888

DIM AS INTEGER pattern0(0)
pattern0(0) = &HFFFF

' precompute all f() values
DIM AS INTEGER  fncache_x1(STATIC arr_sz_x, arr_sz_z)
DIM AS INTEGER  fncache_y1(STATIC arr_sz_x, arr_sz_z)
DIM AS SINGLE   fncache_y (STATIC arr_sz_x, arr_sz_z)

z = zb
FOR iz AS INTEGER = 0 TO arr_sz_z - 1

    LOCATE 3,1 : PRINT "Precomputing fn values ";iz+1;"/";arr_sz_z;", please wait...   "
    x = xa

    FOR ix AS INTEGER = 0 TO arr_sz_x - 1

        x = x + ste_x
        y = FNfun(x, z)

        fncache_y(ix, iz) = y

        GOSUB transform

        fncache_x1(ix, iz) = x1
        fncache_y1(ix, iz) = y1

    NEXT ix

    z = z - ste_z

NEXT iz

'
' main drawing routine
'

PATTERN ,pattern0
COLOR 1
LINE (0,0)-(wx,wy),1,BF

AREA OUTLINE FALSE
COLOR 3,,2

FOR gz AS INTEGER = 0 TO arr_sz_z - prec_z STEP prec_z
    FOR gx AS INTEGER = 0 TO arr_sz_x - prec_x STEP prec_x

        ' PRINT "gx=";gx;", gz=";gz

        FOR goz AS INTEGER = 1 TO prec_z
            FOR gox AS INTEGER = 1 TO prec_x

                ' draw area

                DIM y_avg AS SINGLE = 0
        
                AREA (fncache_x1(gx+gox-1, gz+goz-1), fncache_y1(gx+gox-1, gz+goz-1)) : y_avg = y_avg + fncache_y(gx+gox-1, gz+goz-1)
                AREA (fncache_x1(gx+gox  , gz+goz-1), fncache_y1(gx+gox  , gz+goz-1)) : y_avg = y_avg + fncache_y(gx+gox  , gz+goz-1)
                AREA (fncache_x1(gx+gox  , gz+goz  ), fncache_y1(gx+gox  , gz+goz  )) : y_avg = y_avg + fncache_y(gx+gox  , gz+goz  )
                AREA (fncache_x1(gx+gox-1, gz+goz  ), fncache_y1(gx+gox-1, gz+goz  )) : y_avg = y_avg + fncache_y(gx+gox-1, gz+goz  )

                ' pick color according to height (Y coordinate)

                y_avg = y_avg / 4
                DIM AS SINGLE c =  5 * (y_avg - ya) / (yb-ya) + 3
                DIM AS INTEGER ci = FIX(c)
                ' LOCATE 3,1 : PRINT "COLOR for y_avg ";y_avg;" -> ";c
                IF ci > 7 THEN
                    ci = 7
                END IF
                DIM AS INTEGER cb = ci + 1
                IF cb > 7 THEN
                    cb = 7
                END IF
                COLOR ci,cb,2

                ' pick pattern for some dithering

                DIM AS SINGLE cd = c-FIX(c)
                ' LOCATE 2,1 : PRINT "c=";c;", ci=";ci;", cd=";cd
                IF cd > 0.75 THEN
                    PATTERN , pattern75
                ELSEIF cd > 0.5 THEN
                    PATTERN , pattern50
                ELSEIF cd > 0.25 THEN
                    PATTERN , pattern25
                ELSE
                    PATTERN , pattern0
                END IF

                AREAFILL 0

                '
                ' grid
                '

                IF goz = 1 THEN
                    LINE (fncache_x1(gx+gox-1, gz+goz-1), fncache_y1(gx+gox-1, gz+goz-1)) - (fncache_x1(gx+gox  , gz+goz-1), fncache_y1(gx+gox  , gz+goz-1)), 2
                END IF

                IF gox = 1 THEN
                    LINE (fncache_x1(gx+gox-1, gz+goz-1), fncache_y1(gx+gox-1, gz+goz-1)) - (fncache_x1(gx+gox-1, gz+goz  ), fncache_y1(gx+gox-1, gz+goz  )), 2
                END IF

                IF gox = prec_x THEN
                    LINE (fncache_x1(gx+gox  , gz+goz-1), fncache_y1(gx+gox  , gz+goz-1)) - (fncache_x1(gx+gox  , gz+goz  ), fncache_y1(gx+gox  , gz+goz  )), 2
                END IF

            NEXT gox
        NEXT goz
    NEXT gx
NEXT gz


LOCATE 23,1
COLOR 1,0
PRINT "PRESS ANY KEY TO QUIT"

WHILE INKEY$()=""
WEND

END

'
' subroutines (not using SUBs/FUNCTIONs here as this serves as a GOSUB+RETURN test)
'

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

    'PRINT "tf: sx=";sx;", tx=";tx;", A=";A;", B=";B

    ' project

    DIM AS SINGLE tmp2 = zt2 - camera_z
    x1 = camera_x - camera_z * (xt2-camera_x)/tmp2
    y1 = camera_y - camera_z * (yt2-camera_y)/tmp2

    ' 2D translation and scaling
    x1 = trans_x + x1/scale_x
    y1 = trans_y - y1/scale_y

    ' simple clipping
    IF x1<0  THEN x1=0
    IF x1>wx THEN x1=wx
    IF y1<0  THEN y1=0
    IF y1>wy THEN y1=wy

    'PRINT "transform: ";x;y;z;" -> ";x1;y1
RETURN

