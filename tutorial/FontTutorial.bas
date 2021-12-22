'
' system and custom font loading tutorial
'

OPTION EXPLICIT

WINDOW 1, "Font tutorial"

' load a system font

DIM AS FONT_t PTR sysfont = FONT ("Opal.font", 12)

' load a font from a specific directory

DIM AS FONT_t PTR myfont = FONT ("Future.font", 30, "PROGDIR:/Fonts")

' use our custom font

FONT myfont

LOCATE 1, 1

PRINT "Custom font"

' use the system font

FONT sysfont

LOCATE 5, 1

PRINT "System font"

LOCATE 7,1

PRINT "*** PRESS ENTER TO QUIT ***"

WHILE INKEY$=""
    SLEEP
WEND


