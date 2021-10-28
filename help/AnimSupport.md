
# AQB / AnimSupport

:toc:

## GELS INIT

Syntax:

    GELS INIT [ sprRsvd ]

init the GELs animation system. Has to be called on any screen, window or
bitmap once before any of the commands in this module can be used.

Arguments:
    * sprRsvd: bitmask, each bit corresponds to one of the 8 hardware
               sprites. Sprites that have their corresponding bit set in
               this mask will be reserved for VSPRITES, if any.
               Default: &H00 (no sprites will be reserved)

## GELS REPAINT

Syntax:

    GELS REPAINT

repaint all VSPRITEs and BOBs in the current window, screen or bitmap

## BOB()

Syntax:

    BOB             "(" bm ")"

create a new BOB from a bitmap. Important: the bitmap must have been created in contious mode.

Example:

    DIM AS BITMAP\_t PTR gorilla = NULL

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

release all resources allocated by this bob, hide BOB first if still
visible.

## ILBM READ BOB

Syntax:

	ILBM LOAD BOB path [, [bobPtr] [, [scid] [, [pMETA] [, [pPalette] ] ] ] ]

load a bob from an IFF ILBM file.

Arguments:
	* path: pathname of the file to load the bitmap from
	* bobPtr: reference to a BOB pointer, will be used to store the newly created BOB
	* scid: screen id - if given, colors will be applied to it
	* pMETA: pointer to metadata
	* pPalette: pointer to a palette

Example:

	DIM AS BOB\_t PTR dragon = NULL

	ILBM LOAD BOB "PROGDIR:imgs/dragon.iff", bob, 1


