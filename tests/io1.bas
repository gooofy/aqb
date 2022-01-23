OPTION EXPLICIT

DIM AS BYTE     B   = -42
DIM AS UBYTE    ub  = 200
DIM AS INTEGER  i   = -32000
DIM AS UINTEGER ui  = 65000
DIM AS LONG     l   = -100000
DIM AS ULONG    ul  = 100000
DIM AS SINGLE   f   = 3.1415
DIM AS BOOLEAN  myb = TRUE
DIM AS STRING   mys = "Hello World!"

PRINT "io1 (PRINT): ",mys;B;23;ub;i;ui;l;ul;f;myb

PRINT "io1 (WRITE): ";
WRITE mys,B,23,ub,i,ui,l,ul,f,myb

PRINT "Press RETURN..."

DIM AS STRING answer

INPUT answer

PRINT "Thank you."

