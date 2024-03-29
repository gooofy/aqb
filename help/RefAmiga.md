
# AQB / Amiga specific commands

:toc:


## AREA

Syntax:

    AREA [STEP] (x, y)

add a point to an area to be filled


## AREAFILL

Syntax:

    AREAFILL [mode]

fill a polygon defined by AREA statements, mode: 0=regular fill, 1=inverted fill


## AREA OUTLINE

Syntax:

    AREA OUTLINE ( TRUE | FALSE )

enable or disable drawing of AREA polygon outlines

## BITMAP FREE

Syntax:

    BITMAP FREE bm

free allocated resources for a bitmap


## BITMAP MASK

Syntax:

    BITMAP MASK bm

allocate (is not already done) a new mask plane for
the given bitmask and compute its contents.

Allows for transparent background clipping of bm using
minterm &HE0.

Example:

    DIM AS BITMAP_t PTR notesbm=NULL

    ILBM LOAD BITMAP "PROGDIR:notes.ilbm", notesbm

    BITMAP MASK notesbm

    PUT (10, 10), notesbm, &HE0


## BITMAP OUTPUT

Syntax:

    BITMAP OUTPUT bm

redirect drawing commands output to bm


## BITMAP()

Syntax:

    BITMAP "(" width, height, depth [ , cont ] ")"

allocate an offscreen bitmap buffer. If cont is TRUE, planes will be
allocated in one continuous memory space suitable for BOBs.

Example:

    DIM AS BITMAP_t PTR b = BITMAP (64, 64, 2)


## CIRCLE

Syntax:

    CIRCLE [STEP] "(" x "," y ")" "," r [ "," [color] [ "," [start] [ "," [fini] [ "," ratio ] ] ] ]

draw a circle or ellipse.

Arguments:

    * x, y : center coordinates
    * r : radius
    * color : color register to use
    * start, fini : start and end angles of ellipse arc
    * ratio : aspect ratio of ellipse (rx vs ry radius, default: 0.44)


## COLOR

Syntax:

    COLOR [ fg ] ["," [bg] ["," [o] [ "," drmd ] ]]

set foreground, background, area outline pen and or draw mode.

Useful draw mode values include:

    DRMD_JAM1       = 0
    DRMD_JAM2       = 1 : REM default
    DRMD_COMPLEMENT = 2
    DRMD_INVERSVID  = 4

## CLOSE

Syntax:

    CLOSE [ ["#"] expN ( "," ["#"] expM )* ]

close one or more files


## CSRLIN()

Syntax:

    CSRLIN "(" ")"

returns the current text row position

## FONT FREE

Syntax:

    FONT FREE font

free allocated resources for a font

## FONT

Syntax:

    FONT font

set current font to be used for text output

## FONT()

Syntax:

    FONT "(" name "," size [ "," dir ] ")"

load a new font by name and size. If dir is specified the font is loaded
from that directory, otherwise tries to load a system font.

Example:

    DIM AS FONT_t PTR f = FONT ("opal.font", 12)

## FONTSTYLE

Syntax:

    FONTSTYLE style

set font soft styling used for text output. style can be any OR combination of

    * FSF_UNDERLINED = 1
    * FSF_BOLD       = 2
    * FSF_ITALIC     = 4
    * FSF_EXTENDED   = 8


## FONSTYLE()

Syntax:

    FONTSTYLE "(" ")"

return the current soft font styling flags.


## GET

Syntax:

    GET [[STEP] "(" x1 "," y1 ")" ] "-" [STEP] "(" x2 "," y2 ")" "," bm

copy a rectangular region from current window to a background bitmap
buffer.  See BITMAP() for more information on how to allocate a bitmap. Use
PUT to copy a bitmap back onto the screen.

Example:

    DIM AS BITMAP_t PTR b = BITMAP (64, 64, 2)

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


## LINE

Syntax:

    LINE [[STEP] "(" x1 "," y1 ")" ] "-" [STEP] "(" x2 "," y2 ")" [ "," [color] ["," b[f]] ]

draw a line or a box on the current window, "b": A box is drawn, "bf": a
filled box is drawn.  If option STEP is set, coordinates are relative.


## LOCATE XY

Syntax:

    LOCATE XY "(" x "," y ")"

move cursor to coordinates (x/y)


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

Example:

    SUB mousecb (BYVAL wid AS INTEGER, BYVAL button AS BOOLEAN, BYVAL mx AS INTEGER, BYVAL my AS INTEGER, BYVAL ud AS VOID PTR)
        ...
    END SUB

    ON MOUSE CALL mousecb, NULL
    MOUSE ON


## ON MOUSE MOTION CALL

Syntax:

    ON MOUSE MOTION CALL sub

call `sub` when mouse is moved

Example:

    SUB mousemovecb (BYVAL wid AS INTEGER, BYVAL button AS BOOLEAN, BYVAL mx AS INTEGER, BYVAL my AS INTEGER, BYVAL ud AS VOID PTR)
        ...
    END SUB

    ON MOUSE MOTION CALL mousemovecb
    MOUSE MOTION ON


## ON TIMER CALL

Syntax:

    ON TIMER CALL id, t, sub

assign timer #`id` to call `sub` every `t` seconds. After setup
the timer still has to be enabled using the TIMER ON statement.


## ON WINDOW CLOSE CALL

Syntax:

    ON WINDOW CLOSE CALL id "," cb

call subroute cb when the close button on window #id is clicked.

Example:

    SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
        TRACE "window close cb called, wid=";wid
        SYSTEM
    END SUB

    WINDOW 1, "Window close callback demo"
    ON WINDOW CLOSE CALL 1, winCloseCB, NULL


## ON WINDOW NEWSIZE CALL

Syntax:

    ON WINDOW NEWSIZE CALL id "," cb

call subroute cb when window #id is resized.

Example:

    SUB winNewsizeCB (BYVAL wid AS INTEGER, BYVAL w AS INTEGER, BYVAL h AS INTEGER, BYVAL ud AS VOID PTR)
        TRACE "window newsize cb called, wid=";wid
    END SUB

    WINDOW 1, "Window newsize callback demo"
    ON WINDOW NEWSIZE CALL 1, winNewsizeCB, NULL


## ON WINDOW REFRESH CALL

Syntax:

    ON WINDOW REFRESH CALL id "," cb

call subroute cb when window #id needs to be repainted.

Example:

    SUB winRefreshCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
        TRACE "window refresh cb called, wid=";wid
    END SUB

    WINDOW 1, "Window refresh callback demo"
    ON WINDOW REFRESH CALL 1, winRefreshCB, NULL


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

restore line and area pattern to default (solid)


## POINT()

Syntax:

    POINT "(" x "," y ")"

returns the pen number of a point in the current window or bitmap


## POS()

Syntax:

    POS "(" expression ")"

returns the column of the cursor in the current window (expression given is
a dummy value for compatibility reasons, usually 0).


## PSET

Syntax:

    PSET [ STEP ] "(" x "," y ")" [ "," color ]

set a point in the window


## PUT

Syntax:

    PUT [[STEP] "(" x "," y ")" ], bm, [minterm [ "," [ "(" x1 "," y1 ")" ] "-" "(" x2 "," y2 ")" ] ]

copy a bitmap onto the screen.

Useful minterms:

	* &H30 Replace destination area with inverted source.
	* &H50 Replace destination area with an inverted version of itself.
	* &H80 Only put bits into destination where there is a bit in the same position for both source and destination (sieve operation).
	* &HC0 Copy from source to destination.
	* &HE0 Copy from source to destination through mask.

Example:

    DIM AS BITMAP_t PTR b = BITMAP (64, 64, 2)

    GET (0,0)-(63,63), b

    PUT (100, 40), b, &HC0, (20,10)-(40,40)


## SCREEN

Syntax:

    SCREEN screen-id, width, height, depth, mode [, title]

create a new screen

mode is a combination of:

    * AS_MODE_GENLOCK_VIDEO   = &H0002
    * AS_MODE_LACE            = &H0004
    * AS_MODE_DOUBLESCAN      = &H0008
    * AS_MODE_SUPERHIRES      = &H0020
    * AS_MODE_PFBA            = &H0040
    * AS_MODE_EXTRA_HALFBRITE = &H0080
    * AS_MODE_GENLOCK_AUDIO   = &H0100
    * AS_MODE_DUALPF          = &H0400
    * AS_MODE_HAM             = &H0800
    * AS_MODE_EXTENDED_MODE   = &H1000
    * AS_MODE_VP_HIDE         = &H2000
    * AS_MODE_SPRITES         = &H4000
    * AS_MODE_HIRES           = &H8000

## SCREEN CLOSE

Syntax:

    SCREEN CLOSE id

close screen indicated by id

## SLEEP

Syntax:

    SLEEP

Suspend program until next event occurs.


## SOUND

Syntax:

	SOUND [freq], [duration], [volume], [channel]

play a sound of given frequency, duration, volume on the channel specified.
Sound output is non-blocking, this statement just queues the audio request,
i.e. program execution will continue as long as free audio request slots are
available.  See SOUND WAIT for information on how to sync audio output.

All arguments are optional:

	* freq: frequency in Hz (SINGLE), default: play channel voice sample in its original sampled frequency
	* duration: duration in seconds (SINGLE), default: play complete channel voice sample once
	* volume: playback volume 0-255, default: 127
	* channel: channel to use, default: 0


## SOUND START

Syntax:

	SOUND START [channel]

resume audio output on the specified channel (or all channels, if not specified).

	* channel: audio channel to ststart, default: start all channels


## SOUND STOP

Syntax:

	SOUND STOP [channel]

stop audio output on the specified channel (or all channels, if not specified). Further
audio requests will be queued until SOUND START is used to resume playback.

Warning: program execution will be blocked if sound output is stopped and
request slots are exhausted.

	* channel: audio channel to stop, default: stop all channels


## SOUND WAIT

Syntax:

	SOUND WAIT [channel]

wait for audio playback to finish on the given channel if specified, on all
channels otherwise

	* channel: audio channel to wait for, default: wait for all channels


## TAGITEMS()

Syntax:

    TAGITEMS "(" tag [ "," value [...] ] ")"

allocate and initialize a new array of tagitems, suitable for passing as
taglists to OS calls.

DEALLOCATE() can be used to free the list once no longer used.

IMPORTANT: always terminate the tag list with TAG_DONE!

Example:

    REM allocate a list of 3 tag items:

    DIM AS TAGITEM_t PTR mytagitems = TAGITEMS (42, 2, 23, 0, TAG_DONE)


## TAGS()

Syntax:

    TAGS "(" tag [ "," tag [...] ] ")"

allocate and initialize a new array of tags.

DEALLOCATE() can be used to free the list once no longer used.

IMPORTANT: always terminate the tag list with TAG_END!

Example:

    REM allocate a list of 4 tags items:

    DIM AS ULONG PTR mytags = TAGS (1,2,3,TAG_END)


## TEXTEXTEND

Syntax:

    TEXTEXTEND s, w, h

compute the width and height of s in pixels, taking the current font into
account. w and h are BYREF arguments that will contain the result.


## TEXTWIDTH()

Syntax:

    TEXTWIDTH "(" s ")"

return the width of s in pixels, taking the current font into account.


## TIMER ON|OFF

Syntax:

    TIMER (ON|OFF) id

Enable or disable events from timer #`id`


## VWAIT

Syntax:

    VWAIT

Wait for vertical blank to occur


## WAVE

Syntax:

	WAVE channel, w

use waveform w for the given audio channel


## WAVE()

Syntax:

    WAVE "(" wd "," [oneShotHiSamples] "," [repeatHiSamples] "," [samplesPerHiCycle] "," [samplesPerSec] "," [ctOctave] "," [volume]  )

Create a new waveform object in CHIP memory based on the sample data provided.
Arguments (everything apart from wd is optional):

    * wd : Array of samples, one BYTE per sample, mono
    * oneShotHiSamples: samples in the high octave 1-shot part, default: 0
    * repeatHiSamples: samples in the high octave repeat part, default: 32
    * samplesPerHiCycle: samples/cycle in high octave, default: 32
    * samplesPerSec: data sampling rate, default: 8192
    * ctOctave: number of octaves (ATM only one octave is supported by the runtime), default: 1
    * volume: playback volume in Fixed format, default: &H10000

Example:

	DIM AS BYTE wavedata(31)
	FOR i AS INTEGER = 0 TO 15
		wavedata(i)=127
		wavedata(i+16) = -127
	NEXT i

	DIM AS WAVE_t PTR w = WAVE (wavedata)


## WAVE FREE

Syntax:

	WAVE FREE w

Free resources allocated for waveform w. Use with caution: ensure w isn't used
in any audio channel anymore.


## WINDOW

Syntax:

    WINDOW id [ "," [ title ] [ "," [ "(" x1 "," y1 ")" "-" "(" x2 "," y2 ")" ] [ "," [flags] [ "," screen-id ]]]]

Create and activate a new window, make it the new output window.

flags is an OR combination of:

    * AW_FLAG_SIZEGADGET     = &H00000001
    * AW_FLAG_DRAGBAR        = &H00000002
    * AW_FLAG_DEPTHGADGET    = &H00000004
    * AW_FLAG_CLOSEGADGET    = &H00000008
    * AW_FLAG_SIZEBRIGHT     = &H00000010
    * AW_FLAG_SIZEBBOTTOM    = &H00000020
    * AW_FLAG_REFRESHBITS    = &H000000C0
    * AW_FLAG_SMART_REFRESH  = &H00000000
    * AW_FLAG_SIMPLE_REFRESH = &H00000040
    * AW_FLAG_SUPER_BITMAP   = &H00000080
    * AW_FLAG_OTHER_REFRESH  = &H000000C0
    * AW_FLAG_BACKDROP       = &H00000100
    * AW_FLAG_REPORTMOUSE    = &H00000200
    * AW_FLAG_GIMMEZEROZERO  = &H00000400
    * AW_FLAG_BORDERLESS     = &H00000800
    * AW_FLAG_ACTIVATE       = &H00001000
    * AW_FLAG_RMBTRAP        = &H00010000
    * AW_FLAG_NOCAREREFRESH  = &H00020000
    * AW_FLAG_NW_EXTENDED    = &H00040000
    * AW_FLAG_NEWLOOKMENUS   = &H00200000

default flags: AW_FLAG_SIZEGADGET OR AW_FLAG_DRAGBAR OR AW_FLAG_DEPTHGADGET OR AW_FLAG_CLOSEGADGET OR AW_FLAG_SMART_REFRESH OR AW_FLAG_GIMMEZEROZERO OR AW_FLAG_ACTIVATE


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
    * 15: screen visible width
    * 16: screen visible height

