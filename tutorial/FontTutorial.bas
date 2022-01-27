'
' system and custom font loading tutorial
'

OPTION EXPLICIT

WINDOW 1, "Font tutorial"

' load a system font

DIM AS FONT_t PTR emeraldfont = FONT ("emerald.font", 17)

' load a font from a specific directory

DIM AS FONT_t PTR myfont = FONT ("future.font", 30, "PROGDIR:Fonts")

' use our custom font

FONT myfont

LOCATE 1, 1

PRINT "future.font"

' use the system font

FONT emeraldfont

LOCATE 4, 1

PRINT "emerald.font"

LOCATE 7,1

' use pixel resolution text width and cursor location
' for font-agnostic text centering

SUB CPRINT (BYVAL yoff AS INTEGER, BYVAL s AS STRING)
    DIM AS INTEGER w = TEXTWIDTH(s)
    LOCATE XY (320-w/2, yoff)
    PRINT s;    
END SUB

CPRINT 100, "centered text in emerald.font"

FONT myfont
CPRINT 140, "future.font centered"

' fonts support soft styling

FONT emeraldfont
FONTSTYLE FSF_BOLD OR FSF_UNDERLINED
CPRINT 180, "*** PRESS ENTER TO QUIT ***"

WHILE INKEY$=""
    SLEEP
WEND


