REM A simple GadTools Button Gadget

OPTION EXPLICIT

IMPORT GadToolsSupport

REM GTGADGETUP callback

SUB buttonCB (BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    
    TRACE "buttonCB called."
    
    SYSTEM
    
END SUB

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
    TRACE "window close cb called, wid=";wid
    SYSTEM
END SUB

WINDOW 1, "A simple button gadget"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create a simple button gadget

DIM AS GTBUTTON PTR gadget

gadget = NEW GTBUTTON ("QUIT", (15, 26)-(114, 56))

REM connect our callback

gadget->gadgetup_cb = buttonCB

REM deploy our gadgets to the current window

GTGADGETS DEPLOY

REM regular event loop

WHILE TRUE
    SLEEP
WEND


