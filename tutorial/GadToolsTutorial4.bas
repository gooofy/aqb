REM GadgetTutorial4: STRING, INTEGER, NUMBER widgets

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS GTSTRING  PTR strgadget
DIM SHARED AS GTINTEGER PTR intgadget
DIM SHARED AS GTNUMBER  PTR numgadget
DIM SHARED AS GTBUTTON  PTR button

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
    SYSTEM
END SUB

SUB strcb(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    
    TRACE "String gadget UP cb"
    TRACE "str is: "; strgadget->str
    
END SUB

SUB intcb(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    
    TRACE "Integer gadget UP cb"
    TRACE "num is: "; intgadget->number
    numgadget->number = intgadget->number
    
END SUB

SUB reset(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    strgadget->str = "ABC abc 123"
    intgadget->number = 42
    numgadget->number = 42
END SUB

WINDOW 1, "GadgetTutorial 4"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create our gadgets

strgadget = NEW GTSTRING ("String",  ( 75, 20)-(235, 32))
strgadget->str = "ABC abc 123"
strgadget->maxChars = 40

intgadget = NEW GTINTEGER ("Integer",  ( 75, 36)-(235, 48))
intgadget->number = 42

numgadget = NEW GTNUMBER ("Number", 42,  (310, 36)-(360, 48))
numgadget->border = TRUE

button = NEW GTBUTTON ("Reset", (100, 60)-(214, 92))

GTGADGETS DEPLOY

button->gadgetup_cb = reset
strgadget->gadgetup_cb = strcb
intgadget->gadgetup_cb = intcb

WHILE TRUE
    SLEEP
WEND

