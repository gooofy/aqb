
# AQB / AnimSupport

:toc:

## SPRITE()

Syntax:

    SPRITE "(" bm [ "," "(" x1 "," y1 ")" "-" "(" x2 "," y2 ")" ]")"

create a new SPRITE from a bitmap.

Example:

    DIM AS BITMAP_t PTR spbm = NULL

    ILBM LOAD BITMAP "sprite.iff", spbm

    DIM AS SPRITE_t PTR gsprite

    gsprite = SPRITE(spbm)

## SPRITE SHOW

Syntax:

    SPRITE SHOW spnum "," sprite

use hardware sprite #spnum to display sprite

## SPRITE HIDE

Syntax:

    SPRITE HIDE spnum

release hardware sprite #spnum

## SPRITE MOVE

Syntax:

    SPRITE MOVE spnum,  "(" x "," y ")"

move hardware sprite #spnum to a new location

## SPRITE FREE

Syntax:

    SPRITE FREE sprite

free resources allocated for sprite

## ILBM LOAD SPRITE

Syntax:

	ILBM LOAD SPRITE path, spritePtr [, [scid] [, [pMETA] [, [pPalette] ] ] ]

load a sprite from an IFF ILBM file.

Arguments:
 * path: pathname of the file to load the bitmap from
 * spritePtr: reference to a SPRITE_t pointer, will be used to store the newly created SPRITE
 * scid: screen id - if given, colors will be applied to it
 * pMETA: pointer to metadata
 * pPalette: pointer to a palette

Example:

	DIM AS SPRITE_t PTR sp = NULL

	ILBM LOAD SPRITE "PROGDIR:imgs/dragon.iff", sp

## POINTER SPRITE

Syntax:

    POINTER SPRITE sprite [, [xoffset], [yoffset]]

use sprite as a custom mouse pointer in the current window
default offset is 0, 0 and means the reference point is in the upper left corner
negative values moves the pointer to left and up (not the reference point)
-8, -8 moves the middle of a 16 by 16 pixel pointer to the reference point

## POINTER CLEAR

Syntax:

    POINTER CLEAR

make the current window use the default system mouse pointer

## GELS INIT

Syntax:

    GELS INIT [ sprRsvd ]

init the GELs animation system. Has to be called on any screen, window or
bitmap once before any of the commands in this module can be used.

Arguments:
 * sprRsvd: bitmask, each bit corresponds to one of the 8 hardware sprites. 
            Sprites that have their corresponding bit set in this mask will be reserved for VSPRITES, 
            if any.
 Default: &H00 (no sprites will be reserved)

## GELS REPAINT

Syntax:

    GELS REPAINT

repaint all VSPRITEs and BOBs in the current window, screen or bitmap

## BOB()

Syntax:

    BOB "(" bm  [ "," "(" x1 "," y1 ")" "-" "(" x2 "," y2 ")" ] ")"

create a new BOB from a bitmap. Important: the bitmap must have been created in continuous mode.

Example:

    DIM AS BITMAP_t PTR gorilla = NULL

    ILBM LOAD BITMAP "gorilla.iff", gorilla,,,,TRUE

    DIM AS BOB_t PTR gbob

    gbob = BOB (gorilla)

## BOB SHOW

Syntax:

    BOB SHOW bob

make bob visible on the current window, screen or bitmap

## BOB HIDE

Syntax:

    BOB HIDE bob

remove bob from current window, screen or bitmap

## BOB MOVE

Syntax:

    BOB MOVE bob, "(" x "," y ")"

move bob to a new location

## BOB FREE

Syntax:

    BOB FREE bob

release all resources allocated by this bob, hide BOB first if still visible.

## ILBM LOAD BOB

Syntax:

	ILBM LOAD BOB path, bobPtr [, [scid] [, [pMETA] [, [pPalette] ] ] ]

load a bob from an IFF ILBM file.

Arguments:
 * path: pathname of the file to load the bitmap from
 * bobPtr: reference to a BOB_t pointer, will be used to store the newly created BOB
 * scid: screen id - if given, colors will be applied to it
 * pMETA: pointer to metadata
 * pPalette: pointer to a palette

Example:

	DIM AS BOB_t PTR bob = NULL

	ILBM LOAD BOB "PROGDIR:imgs/dragon.iff", bob, 1


