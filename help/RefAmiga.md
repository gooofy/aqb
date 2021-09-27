
# AQB / Amiga specific commands

:toc:


## AREA

Syntax:

    AREA [STEP] (x, y)

add a point to an area to be filled


## AREAFILL

Syntax:

    AREAFILL [mode]

fill a polygon defined by AREA statements, mode: 0=regular fill, 1=inverted
fill


## AREA OUTLINE

Syntax:

    AREA OUTLINE ( TRUE | FALSE )

enable or disable drawing of AREA polygon outlines

## BLIT FREE

Syntax:

    BLIT FREE ptr

free allocated ressources for a blit buffer

## BLIT()

Syntax:

    BLIT "(" width, height, depth ")"

allocate an offscreen blit (i.e. BitMap) buffer

Example:

    DIM AS VOID PTR b = BLIT (64, 64, 2)

## COLOR

Syntax:

    COLOR [ fg ] ["," [bg] ["," o]]

set foreground, background and or area outline pen


## CLOSE

Syntax:

    CLOSE [ ["#"] expN ( "," ["#"] expM )* ]

close one or more files


## CLS

Syntax:

    CLS

Clear the current output window and set the cursor position to the upper
left corner


## CSRLIN()

Syntax:

    CSRLIN "(" ")"

returns the current text row position

## GET

Syntax:

    GET [[STEP] "(" x1 "," y1 ")" ] "-" [STEP] "(" x2 "," y2 ")" "," blit

copy a rectangular region from current window to a background blit buffer.
See BLIT() for more information on how to allocate a blit buffer. Use PUT
to copy a blit buffer back onto the screen.

Example:

    DIM AS VOID PTR b = BLIT (64, 64, 2)

    GET (0,0)-(63,63), b

    PUT (100, 40), b

## INKEY$()

Syntax:

    INKEY$ "(" ")"

returns a character entered from the keyboard

Non-ascii key codes:

    * 28 Up cursor
    * 29 Down cursor
    * 30 Right cursor
    * 31 Left cursor
    * 129 F1
    * 130 F2
    * 131 F3
    * 132 F4
    * 133 F5
    * 134 F6
    * 135 F7
    * 136 F8
    * 137 F9
    * 138 F10


## INPUT

Syntax:

    INPUT [ ";" ] [ prompt (";" | ",") ] expDesignator ( "," expDesignator* )

read input from the keyboard, store values in the variables given.


## LINE

Syntax:

    LINE [[STEP] "(" x1 "," y1 ")" ] "-" [STEP] "(" x2 "," y2 ")" [ "," [color] ["," b[f]] ]

draw a line or a box on the current window, "b": A box is drawn, "bf": a
filled box is drawn.  If option STEP is set, coordinates are relative.


## LINE INPUT

Syntax:

    LINE INPUT [ ";" ] [ stringLiteral ";" ] expDesignator

request a STRING keyboard entry from a program user.


## LOCATE

Syntax:

    LOCATE [ row ] [ "," col ]

move cursor to col / row


## MOUSE()

Syntax:

    MOUSE "(" n ")"

return information about the current status of the mouse. Mouse positions
are reported relative to the upper left corner of the current output
window.

Valid values for n:

    * 0: number of times the left mouse button was pressed since the last MOUSE(0) call:
    ** 0: left mouse button was and is not pressed
    ** 1: left mouse button was pressed once but isn't held down now
    ** 2: left mouse button was pressed twice but isn't held down now
    ** -1: left mouse button was pressed once and is still held down
    ** -2: left mouse button was pressed twice and is still held down
    * 1: current mouse X location
    * 2: current mouse Y location
    * 3: mouse X when last time a button was pressed
    * 4: mouse Y when last time a button was pressed
    * 5: mouse X when last time a button was released
    * 6: mouse Y when last time a button was released

## MOUSE ( ON | OFF )

Syntax:

    MOUSE ON
    MOUSE OFF

enable or disable mouse button events

## MOUSE MOTION ( ON | OFF )

Syntax:

    MOUSE MOTION ON
    MOUSE MOTION OFF

enable or disable mouse move events

## ON MOUSE CALL

Syntax:

    ON MOUSE CALL sub

call `sub` on left mouse button press and release


## ON MOUSE MOTION CALL

Syntax:

    ON MOUSE MOTION CALL sub

call `sub` when mouse is moved


## ON TIMER CALL

Syntax:

    ON TIMER CALL id, t, sub

assign timer #`id` to call `sub` every `t` seconds. After setup
the timer still has to be enabled using the TIMER ON statement.


## OPEN

Syntax:

    OPEN filename FOR ( RANDOM | INPUT | OUTPUT | APPEND | BINARY ) [ ACCESS ( READ [WRITE] | WRITE ) ] AS ["#"] f [LEN = rln]

open a file for input or output


## PAINT

Syntax:

    PAINT [STEP] "(" x "," y ")" [ "," paintColor [ "," outlineColor ] ]

flood fill an enclosed area surrounded by outlineColor with the specified
color


## PALETTE

Syntax:

    PALETTE n, red, green, blue

change the color palette entry for pen number n. The red, green and blue
arguments are be floating point values in the 0.0 ... 1.0 range.


## PATTERN

Syntax:

    PATTERN [ lineptrn ] [ "," areaptrn ]

change pattern used to draw lines and areas.

    * lineptrn: 16 bit integer that defines the pattern for lines
    * areaptrn: array of 16 bit integers, number of elements must be a power of 2 (1, 2, 4, 8, ...)


## PATTERN RESTORE

Syntax:

    PATTERN RESTORE

Restore line and area pattern to default (solid)


## POS()

Syntax:

    POS "(" expression ")"

returns the column of the cursor in the current window (expression given is
a dummy value for compatibility reasons, usually 0).


## PRINT

Syntax:

    PRINT [ "#" expFNo "," ]  [ expression ( [ ";" | "," ] expression )* ]

print the listed expressions to the screen or a file (if expFNo is given). ";" means no space, "," means
skip to next 9 col tab, ";" or "," at the end of the line mean no newline
is printed.


## PSET

Syntax:

    PSET [ STEP ] "(" x "," y ")" [ "," color ]

set a point in the window


## PUT

Syntax:

    PUT [[STEP] "(" x "," y ")" ], blit, [minterm [ "," [ "(" x1 "," y1 ")" ] "-" "(" x2 "," y2 ")" ] ]

copy a blit buffer onto the screen.

Useful minterms:

	* 0x30 Replace destination area with inverted source.
	* 0x50 Replace destination area with an inverted version of itself.
	* 0x80 Only put bits into destination where there is a bit in the same position for both source and destination (sieve operation).
	* 0xC0 Copy from source to destination.

Example:

    DIM AS VOID PTR b = BLIT (64, 64, 2)

    GET (0,0)-(63,63), b

    PUT (100, 40), b, &HC0, (20,10)-(40,40)


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


## SCREEN CLOSE

Syntax:

    SCREEN CLOSE id

close screen indicated by id

## SLEEP

Syntax:

    SLEEP

Suspend program until next event occurs.


## SLEEP FOR

Syntax:

    SLEEP FOR seconds

Suspend program for the specified number of seconds (floating point value,
so fractions of seconds are supported).


## TIMER ON|OFF

Syntax:

    TIMER (ON|OFF) id

Enable or disable events from timer #`id`


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


## WINDOW CLOSE

Syntax:

    WINDOW CLOSE id

close window indicated by id.


## WINDOW OUTPUT

Syntax:

    WINDOW OUTPUT id

make window indicated by id the current output window.


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

