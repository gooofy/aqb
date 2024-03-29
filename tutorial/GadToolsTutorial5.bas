REM GadgetTutorial5: BevelBox, message callback, dynamic gadget layout

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS CGTGadget PTR button_close
DIM SHARED AS INTEGER winw, winh

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    SYSTEM
END SUB

SUB close_cb(BYVAL g AS CGTGadget PTR, BYVAL code AS UINTEGER)
    system
END SUB

REM dynamic, font-sensitive layout
SUB layout (BYVAL w AS INTEGER, BYVAL h AS INTEGER)

    REM calculate close button coordinates and size
    REM make it sit centered at the bottom of the window

    DIM AS INTEGER gw, gh
    TEXTEXTEND "Close", gw, gh
    gw = gw + 12
    gh = gh + 6
    DIM AS INTEGER x=w/2-gw/2, y=h-6-gh

    'GTGADGETS FREE

    button_close->x1 = x
    button_close->y1 = y
    button_close->x2 = x+gw-1
    button_close->y2 = y+gh-1

    GTGADGETS DEPLOY


END SUB

SUB winRefreshCB (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    TRACE "refresh cb called"

    GTG DRAW BEVEL BOX (6,5)-(winw-7, winh-26), FALSE
    GTG DRAW BEVEL BOX (7,6)-(winw-8, winh-27), TRUE

    LOCATE XY (12, 9) : PRINT " Custom Group Border "

END SUB

SUB winNewsizeCB (BYVAL wid AS INTEGER, BYVAL w AS INTEGER, BYVAL h AS INTEGER, BYVAL ud AS ANY PTR)
    TRACE "newsize cb called, size is: ";w;"x";h
    CLS
    winw = w
    winh = h
    layout w, h
    winRefreshCB wid, NULL
END SUB

WINDOW 1, "GadgetTutorial 5",,AW_FLAG_SIZEGADGET OR AW_FLAG_DRAGBAR OR AW_FLAG_CLOSEGADGET OR AW_FLAG_SIMPLE_REFRESH OR AW_FLAG_DEPTHGADGET OR AW_FLAG_GIMMEZEROZERO

ON WINDOW CLOSE CALL 1, winCloseCB
ON WINDOW NEWSIZE CALL 1, winNewsizeCB
ON WINDOW REFRESH CALL 1, winRefreshCB

button_close = NEW CGTButton ("Close", (0, 0)-(0, 0))

winNewsizeCB 1, WINDOW(2), WINDOW(3), NULL

button_close->gadgetup_cb = close_cb

WHILE TRUE
    SLEEP
WEND

