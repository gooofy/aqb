REM GadgetTutorial4: STRING, INTEGER, NUMBER widgets

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS CGTString  PTR strgadget
DIM SHARED AS CGTInteger PTR intgadget
DIM SHARED AS CGTNumber  PTR numgadget
DIM SHARED AS CGTButton  PTR button

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    SYSTEM
END SUB

SUB strcb(BYVAL g AS CGTGadget PTR, BYVAL code AS UINTEGER)

    TRACE "String gadget UP cb"
    TRACE "str is: "; strgadget->str

END SUB

SUB intcb(BYVAL g AS CGTGadget PTR, BYVAL code AS UINTEGER)

    TRACE "Integer gadget UP cb"
    TRACE "num is: "; intgadget->number
    numgadget->number = intgadget->number

END SUB

SUB reset(BYVAL g AS CGTGadget PTR, BYVAL code AS UINTEGER)
    strgadget->str = "ABC abc 123"
    intgadget->number = 42
    numgadget->number = 42
END SUB

WINDOW 1, "GadgetTutorial 4"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create our gadgets

strgadget = NEW CGTString ("String",  ( 75, 20)-(235, 32))
strgadget->str = "ABC abc 123"
strgadget->maxChars = 40

intgadget = NEW CGTInteger ("Integer",  ( 75, 36)-(235, 48))
intgadget->number = 42

numgadget = NEW CGTNumber ("Number", 42,  (310, 36)-(360, 48))
numgadget->border = TRUE

button = NEW CGTButton ("Reset", (100, 60)-(214, 92))

GTGADGETS DEPLOY

button->gadgetup_cb = reset
strgadget->gadgetup_cb = strcb
intgadget->gadgetup_cb = intcb

WHILE TRUE
    SLEEP
WEND

