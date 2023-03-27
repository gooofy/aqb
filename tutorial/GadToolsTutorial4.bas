REM GadgetTutorial4: STRING, INTEGER, NUMBER widgets

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS GTSTRING  PTR strgadget
'DIM SHARED AS GTINTEGER PTR intgadget
'DIM SHARED AS GTNUMBER  PTR numgadget
DIM SHARED AS GTBUTTON  PTR button

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
    SYSTEM
END SUB

SUB strcb(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    
    TRACE "String gadget UP cb"
    TRACE "str is: "; strgadget->str
    
END SUB

'SUB intcb(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
'    
'    TRACE "Integer gadget UP cb"
'    TRACE "num is: "; GTGNUM(intgadget)
'    GTG MODIFY numgadget, GTNM_Number, GTGNUM(intgadget), TAG_DONE
'    
'END SUB

SUB reset(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    strgadget->str = "ABC abc 123"
    'GTG MODIFY intgadget, GTIN_Number, 42, TAG_DONE
    'GTG MODIFY numgadget, GTNM_Number, 42, TAG_DONE
END SUB

WINDOW 1, "GadgetTutorial 4"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create our gadgets

strgadget = NEW GTSTRING ("String",  ( 75, 20)-(235, 32))
strgadget->str = "ABC abc 123"
strgadget->maxChars = 40

'intgadget = GTGADGET (INTEGER_KIND, ( 75, 36)-(235, 48), "Integer", 0, 2,_
'GTIN_Number, 42, TAG_DONE)

'numgadget = GTGADGET (NUMBER_KIND,  (310, 36)-(360, 48), "Number", 0, 3,_
'GTNM_Number, 42, GTNM_Border, TRUE, TAG_DONE)

button = NEW GTBUTTON ("Reset", (100, 60)-(214, 92))

GTGADGETS DEPLOY

button->gadgetup_cb = reset
strgadget->gadgetup_cb = strcb
' numgadget->gadgetup_cb = numcb

WHILE TRUE
    SLEEP
WEND

