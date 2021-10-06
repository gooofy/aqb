REM
REM IFF ILBM image loading tutorial
REM
REM Load and display a fullscreen IFF image
REM using a borderless window on a custom bitmap screen
REM

OPTION EXPLICIT 
IMPORT IFFSupport 

REM open the IFF file for binary read

OPEN "imgs/hope.iff" FOR BINARY AS #1

REM read meta and palette information

DIM AS ILBM_META_t meta
DIM AS PALETTE_t cmap

ILBM LOAD #1, @meta, @cmap

REM create a custom bitmap, screen AND borderless window

DIM AS BITMAP_t PTR bm = BITMAP (meta.w, meta.h, meta.nPlanes)

SCREEN 1, meta.w, meta.h, meta.nPlanes, meta.viewModes, "IFF DEMO", bm
PALETTE LOAD @cmap

WINDOW 1, ,(0,0)-(meta.w,meta.h), _
AW_FLAG_SIMPLE_REFRESH OR AW_FLAG_BORDERLESS OR AW_FLAG_ACTIVATE, 1

REM load the image contents directly into our custom bitmap

ILBM LOAD #1, @meta,, bm

CLOSE #1

REM WAIT FOR MOUSE BUTTON

SUB doQuit
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

WHILE TRUE
    SLEEP
WEND

