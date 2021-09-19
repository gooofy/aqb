[MAIN]:
# AQB / Amiga specific commands

:toc:

[AREA]:
## AREA

Syntax:

    AREA [STEP] (x, y)

add a point to an area to be filled

[AREAFILL]:
## AREAFILL

Syntax:

    AREAFILL [mode]

fill a polygon defined by AREA statements, mode: 0=regular fill, 1=inverted
fill

[AREA OUTLINE]:
## AREA OUTLINE

Syntax:

    AREA OUTLINE ( TRUE | FALSE )

enable or disable drawing of AREA polygon outlines

[COLOR]:
## COLOR

Syntax:

    COLOR [ fg ] ["," [bg] ["," o]]

set foreground, background and or area outline pen

[CLS]:
## CLS

Syntax:

    CLS

Clear the current output window and set the cursor position to the upper
left corner

[CSRLIN()]:
## CSRLIN()

Syntax:

    CSRLIN "(" ")"

returns the current text row position

[INKEY$()]:
## INKEY$()

Syntax:

    INKEY$ "(" ")"

returns a character entered from the keyboard

.Table (non-ascii) key codes
|===
|Key |Code

|Up cursor
|28

|Down cursor
|29

|Right cursor
|30

|Left cursor
|31

|F1
|129

|F2
|130

|F3
|131

|F4
|132

|F5
|133

|F6
|134

|F7
|135

|F8
|136

|F9
|137

|F10
|138

|===

[INPUT]:
## INPUT

Syntax:

    INPUT [ ";" ] [ prompt (";" | ",") ] expDesignator ( "," expDesignator* )

read input from the keyboard, store values in the variables given.

[LINE]:
## LINE

Syntax:

    LINE [[STEP] "(" x1 "," y1 ")" ] "-" [STEP] "(" x2 "," y2 ")" [ "," [color] ["," b[f]] ]

draw a line or a box on the current window, "b": A box is drawn, "bf": a
filled box is drawn.  If option STEP is set, coordinates are relative.

[LINE INPUT]:
## LINE INPUT

Syntax:

    LINE INPUT [ ";" ] [ stringLiteral ";" ] expDesignator

request a STRING keyboard entry from a program user.

[LOCATE]:
## LOCATE

Syntax:

    LOCATE [ row ] [ "," col ]

move cursor to col / row

[ON TIMER CALL]:
## ON TIMER CALL

Syntax:

    ON TIMER CALL id, t, sub

assign timer #`id` to call `sub` every `t` seconds. After setup
the timer still has to be enabled using the TIMER ON statement.

[PAINT]:
## PAINT

Syntax:

    PAINT [STEP] "(" x "," y ")" [ "," paintColor [ "," outlineColor ] ]

flood fill an enclosed area surrounded by outlineColor with the specified
color

[PALETTE]:
## PALETTE

Syntax:

    PALETTE n, red, green, blue

change the color palette entry for pen number n. The red, green and blue
arguments are be floating point values in the 0.0 ... 1.0 range.

[PATTERN]:
## PATTERN

Syntax:

    PATTERN [ lineptrn ] [ "," areaptrn ]

change pattern used to draw lines and areas.

    * lineptrn: 16 bit integer that defines the pattern for lines
    * areaptrn: array of 16 bit integers, number of elements must be a power of 2 (1, 2, 4, 8, ...)

[PATTERN RESTORE]:
## PATTERN RESTORE

Syntax:

    PATTERN RESTORE

Restore line and area pattern to default (solid)

[POS()]:
## POS()

Syntax:

    POS "(" expression ")"

returns the column of the cursor in the current window (expression given is
a dummy value for compatibility reasons, usually 0).

[PRINT]:
## PRINT

Syntax:

    PRINT  [ expression ( [ ";" | "," ] expression )* ]

print the listed expressions to the screen. ";" means no space, "," means
skip to next 9 col tab, ";" or "," at the end of the line mean no newline
is printed.

[PSET]:
## PSET

Syntax:

    PSET [ STEP ] "(" x "," y ")" [ "," color ]

set a point in the window

[SCREEN]:
## SCREEN

Syntax:

    SCREEN screen-id, width, height, depth, mode [, title]

create a new screen

mode is one of:

    * AS_MODE_LORES                = 1
    * AS_MODE_HIRES                = 2
    * AS_MODE_LORES_LACED          = 3
    * AS_MODE_HIRES_LACED          = 4
    * AS_MODE_HAM                  = 5
    * AS_MODE_EXTRAHALFBRITE       = 6
    * AS_MODE_HAM_LACED            = 7
    * AS_MODE_EXTRAHALFBRITE_LACED = 8

[SCREEN CLOSE]:
## SCREEN CLOSE

Syntax:

    SCREEN CLOSE id

close screen indicated by id

[SLEEP FOR]:
## SLEEP FOR

Syntax:

    SLEEP FOR seconds

Suspend program for the specified number of seconds (floating point value,
so fractions of seconds are supported).

[TIMER ON|OFF]:
## TIMER ON|OFF

Syntax:

    TIMER (ON|OFF) id

Enable or disable events from timer #`id`

[WINDOW]:
## WINDOW

Syntax:

    WINDOW id [ "," [ title ] [ "," [ "(" x1 "," y1 ")" "-" "(" x2 "," y2 ")" ] [ "," [type] [ "," screen-id ]]]]

Create and activate a new window, make it the new output window.

type is an OR combination of:

    * AW_FLAG_SIZE       =  1
    * AW_FLAG_DRAG       =  2
    * AW_FLAG_DEPTH      =  4
    * AW_FLAG_CLOSE      =  8
    * AW_FLAG_REFRESH    = 16
    * AW_FLAG_BACKDROP   = 32
    * AW_FLAG_BORDERLESS = 64

[WINDOW CLOSE]:
## WINDOW CLOSE

Syntax:

    WINDOW CLOSE id

close window indicated by id.

[WINDOW OUTPUT]:
## WINDOW OUTPUT

Syntax:

    WINDOW OUTPUT id

make window indicated by id the current output window.

[WINDOW()]:
## WINDOW()

Syntax:

    WINDOW "(" n ")"

return information about a window. n is one of

    *  0: current active window id
    *  1: current output window id
    *  2: current output window width
    *  3: current output window height
    *  4: current output cursor X
    *  5: current output cursor Y
    *  6: highest color index
    *  7: pointer to current intuition output window
    *  8: pointer to current rastport
    *  9: output file handle
    * 10: foreground pen
    * 11: background pen
    * 12: text width
    * 13: text height
    * 14: input file handle

