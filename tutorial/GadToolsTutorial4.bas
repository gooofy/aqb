REM GadgetTutorial4: STRING, INTEGER, NUMBER widgets

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS GTGADGET_t PTR strgadget, intgadget, numgadget, button

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER)
    SYSTEM
END SUB    

SUB strcb(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    
    TRACE "String gadget UP cb"    
    TRACE "str is: "; GTGBUFFER(strgadget)    
    
END SUB    

SUB intcb(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    
    TRACE "Integer gadget UP cb"    
    TRACE "num is: "; GTGNUM(intgadget)    
    GTG MODIFY numgadget, GTNM_Number, GTGNUM(intgadget), TAG_DONE
    
END SUB    

SUB reset(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    GTG MODIFY strgadget, GTST_String, "ABC abc 123", TAG_DONE
    GTG MODIFY intgadget, GTIN_Number, 42, TAG_DONE
    GTG MODIFY numgadget, GTNM_Number, 42, TAG_DONE
END SUB    

WINDOW 1, "GadgetTutorial 4"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create our gadgets

strgadget = GTGADGET (STRING_KIND,  ( 75, 20)-(235, 32), "String", 0, 1,_
GTST_String, "ABC abc 123", GTST_MaxChars, 40, TAG_DONE)

intgadget = GTGADGET (INTEGER_KIND, ( 75, 36)-(235, 48), "Integer", 0, 2,_
GTIN_Number, 42, TAG_DONE)

numgadget = GTGADGET (NUMBER_KIND,  (310, 36)-(360, 48), "Number", 0, 3,_
GTNM_Number, 42, GTNM_Border, TRUE, TAG_DONE)

button = GTGADGET (BUTTON_KIND, (100, 60)-(214, 92), "Reset", 0, 1, TAG_DONE)

GTGADGETS DEPLOY

ON GTG UP   CALL button, reset, NULL
ON GTG UP   CALL strgadget, strcb, NULL
ON GTG UP   CALL intgadget, intcb, NULL

WHILE TRUE
    SLEEP
WEND

