'
' 3d function plotter
'

IF FRE(-1)+FRE(0) < 36000& THEN
  END
END IF

'CLEAR ,36000&

'DIM winbuf%(14962)
'
'SCREEN 2,640,200,2,2
WINDOW 4,,(0,0)-(631,186),0,2
'
'PALETTE 0,  0,  0,  0
'PALETTE 1, .8,  0,.93
'PALETTE 2,.47,.87,  1
'PALETTE 3,  1, .6,.67
'
'COLOR 2
'
' transformation parameters
' scaling:
sx = 40
sy = 6
sz = 20

' translation:
tx = 0
ty = 0
tz = 0

' rotation:
rx = 15
ry = -45
rz = 0

' viewpoint:
bx = 0
by = 0
bz = -300

' plane translation:
tex = 300
tey = 90

' rotation consts
rx = rx*PI/180
ry = ry*PI/180
rz = rz*PI/180

si_x = SIN(rx) : co_x = COS(rx)
si_y = SIN(ry) : co_y = COS(ry)
si_z = SIN(rz) : co_z = COS(rz)

A = co_y * co_z
B = co_y * si_z
C = -si_y
D = si_x*si_y*co_z - co_x*si_z
E = si_x*si_y*si_z + co_x*co_z
F = si_x*co_y
G = co_x*si_y*co_z + si_x*si_z
H = co_x*si_y*si_z - si_x*co_z
I = co_x*co_y

' start/end/step
CONST sta_x = 3, en_x = -3, ste_x = -.01
CONST sta_z = 4, en_z = -3, ste_z = -.5

' function to plot
FUNCTION FNfun(x AS SINGLE, z AS SINGLE)
    RETURN x*x - z*z
END FUNCTION

' main drawing routine:

FOR z=sta_z TO en_z STEP ste_z
    FOR x=sta_x TO en_x STEP ste_x

      y = FNfun(x,z)

      ' 3D transformation:
      xt1 = sx*x + tx          ' scale
      yt1 = sy*y + ty          ' and translate
      zt1 = sz*z + tz

      xt2 = xt1*A + yt1*B + zt1*C ' rotate
      yt2 = xt1*D + yt1*E + zt1*F
      zt2 = xt1*G + yt1*H + zt1*I

      ' project into 2D
      zwis = zt2 - bz
      xe = bx - bz * (xt2-bx)/zwis
      ye = by - bz * (yt2-by)/zwis

      ' draw
      PSET (tex + xe, tey - ye),1
      LINE (tex+xe,tey-ye+1)-(tex+xe,200),0

    NEXT x
NEXT z

LOCATE 24,0
PRINT "PRESS ANY KEY TO QUIT"

WHILE INKEY$() = ""
WEND

