REM GadTools Tutorial 6: ListView, MX, Palette, Cycle

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS CGTListView PTR lv
DIM SHARED AS CGTMX PTR mx
DIM SHARED AS CGTCycle PTR cy
DIM SHARED AS CGTPalette PTR pl

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS ANY PTR)
    SYSTEM
END SUB

SUB lvcb(BYVAL g AS CGTGadget PTR, BYVAL code AS UINTEGER)
    TRACE "LV cb: code=";code;", selected=";lv->selected
    cy->active = lv->selected
    mx->active = lv->selected
END SUB

SUB mxcb(BYVAL g AS CGTGadget PTR, BYVAL code AS UINTEGER)
    TRACE "MX cb: code=";code;", active="; mx->active
    cy->active = mx->active
    lv->selected = mx->active
END SUB

SUB plcb(BYVAL g AS CGTGadget PTR, BYVAL code AS UINTEGER)
    TRACE "PL cb: code=";code;", color=";pl->color
END SUB

SUB cycb(BYVAL g AS CGTGadget PTR, BYVAL code AS UINTEGER)
    TRACE "CY cb: code=";code;", active="; cy->active
    mx->active = cy->active
    lv->selected = mx->active
END SUB

WINDOW 1, "GadTools Tutorial 6"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create a ListView gadget

DIM AS CExecList choices = CExecList(NT_USER)

choices.AddNode(NEW CExecNode (1,,,"First"))
choices.AddNode(NEW CExecNode (2,,,"Second"))
choices.AddNode(NEW CExecNode (3,,,"Third"))

lv = NEW CGTListView ("ListView", @choices, ( 75, 20)-(235, 132))
lv->makeVisible=2
lv->readOnly = FALSE
lv->selected = 0

REM create a MX gadget

DIM AS STRING mxlabels(STATIC 3)
mxlabels(0)="First"
mxlabels(1)="Second"
mxlabels(2)="Third"
mxlabels(3)=NULL

mx = NEW CGTMX ("MX", @mxlabels(0), (350, 20)-(450, 42))

REM create a palette gadget

pl = NEW CGTPalette ("Palette", 4, (490, 20)-(550, 100))

REM create a cycle gadget

cy = NEW CGTCycle ("Cycle", @mxlabels(0), (350, 100)-(450, 114))

GTGADGETS DEPLOY

REM message handling

lv->gadgetup_cb = lvcb
mx->gadgetdown_cb = mxcb
pl->gadgetup_cb = plcb
cy->gadgetup_cb = cycb

WHILE TRUE
    SLEEP
WEND

