OPTION EXPLICIT

DIM AS BYTE     b   = -42
DIM AS UBYTE    ub  = 200
DIM AS INTEGER  i   = -32000
DIM AS UINTEGER ui  = 65000
DIM AS LONG     l   = -100000
DIM AS ULONG    ul  = 100000
DIM AS SINGLE   f   = 3.1415
DIM AS BOOLEAN  myb = TRUE

' _debug_puts("iotest") : _debug_putnl

PRINT "i/o Test: ",b;23;ub;i;ui;l;ul;f;myb

PRINT "Press any key..."

WHILE INKEY$ = ""
WEND

PRINT "Thank you."

