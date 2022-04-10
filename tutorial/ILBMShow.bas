REM
REM IFF ILBM image loading tutorial
REM
REM Load and display a fullscreen IFF image
REM using a borderless window on a custom bitmap screen
REM

OPTION EXPLICIT
IMPORT IFFSupport

DIM AS STRING picFileName = "PROGDIR:imgs/hope.iff"

REM read meta and palette information only for now

DIM AS ILBM_META_t meta
DIM AS PALETTE_t cmap

ILBM LOAD BITMAP picFileName,,, @meta, @cmap

REM create a matching custom bitmap, screen and borderless window

DIM AS BITMAP_t PTR bm = BITMAP (meta.w, meta.h, meta.nPlanes)
SCREEN 1, meta.w, meta.h, meta.nPlanes, meta.viewModes, "IFF DEMO", bm
PALETTE LOAD @cmap
WINDOW 1, ,(0,0)-(meta.w,meta.h), _
AW_FLAG_SIMPLE_REFRESH OR AW_FLAG_BORDERLESS OR AW_FLAG_ACTIVATE, 1

REM now load the image body into our custom screen bitmap

ILBM LOAD BITMAP "PROGDIR:imgs/hope.iff",bm

REM WAIT FOR MOUSE BUTTON

SUB doQuit (BYVAL wid AS INTEGER, BYVAL b AS BOOLEAN, BYVAL mx AS INTEGER, BYVAL my AS INTEGER, BYVAL ud AS VOID PTR)
    SYSTEM
END SUB
ON MOUSE CALL doQuit
MOUSE ON

WHILE TRUE
    SLEEP
WEND

