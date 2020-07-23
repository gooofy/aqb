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
'WINDOW 4,,(0,0)-(631,186),0,2
'
'PALETTE 0,  0,  0,  0
'PALETTE 1, .8,  0,.93
'PALETTE 2,.47,.87,  1
'PALETTE 3,  1, .6,.67
'
'COLOR 2
'
'' transformation parameters
'' scaling:
'sx = 40
'sy = 6
'sz = 20
'
'' translation:
'tx = 0
'ty = 0
'tz = 0
'
'' rotation:
'rx = 15
'ry = -45
'rz = 0
'
'' viewpoint:
'bx = 0
'by = 0
'bz = -300
'
'' plane translation:
'tex = 300
'tey = 90
'
'' rotation consts
'rx = rx*PI/180
'ry = ry*PI/180
'rz = rz*PI/180
'
'si.x = SIN(rx) : co.x = COS(rx)
'si.y = SIN(ry) : co.y = COS(ry)
'si.z = SIN(rz) : co.z = COS(rz)
'
'A = co.y * co.z
'B = co.y * si.z
'C = -si.y
'D = si.x*si.y*co.z - co.x*si.z
'E = si.x*si.y*si.z + co.x*co.z
'F = si.x*co.y
'G = co.x*si.y*co.z + si.x*si.z
'H = co.x*si.y*si.z - si.x*co.z
'I = co.x*co.y
'
'' start/end/step
'sta.x = 3 : en.x = -3 : ste.x = -.01
'sta.z = 4 : en.z = -3 : ste.z = -.5
'
'' function to plot
'DEF FNfun(x,z) = x*x - z*z
'
'
'' main drawing routine:
'
'  FOR z=sta.z TO en.z STEP ste.z
'    FOR x=sta.x TO en.x STEP ste.x
'
'      y = FNfun(x,z)
'
'      ' 3D transformation:
'      xt1 = sx*x + tx          ' scale
'      yt1 = sy*y + ty          ' and translate
'      zt1 = sz*z + tz
'
'      xt2 = xt1*A + yt1*B + zt1*C ' rotate
'      yt2 = xt1*D + yt1*E + zt1*F
'      zt2 = xt1*G + yt1*H + zt1*I
'
'      ' project into 2D
'      zwis = zt2 - bz
'      xe = bx - bz * (xt2-bx)/zwis
'      ye = by - bz * (yt2-by)/zwis
'
'      ' draw
'      PSET (tex + xe, tey - ye)
'      LINE (tex+xe,tey-ye+1)-(tex+xe,200),0
'
'    NEXT x
'  NEXT z
'
'WHILE INKEY$() = ""
'WEND
