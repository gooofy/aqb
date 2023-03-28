REM GadTools Tutorial 6: ListView, MX, Palette, Cycle

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

'DIM SHARED AS GTGADGET_t PTR lv, pl
DIM SHARED AS GTMX PTR mx
DIM SHARED AS GTCYCLE PTR cy
DIM SHARED AS GTPALETTE PTR pl

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
    SYSTEM
END SUB    

'SUB lvcb(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
'    TRACE "LV cb: wid=";wid;", gid=";gid;", code=";code    
'END SUB    

SUB mxcb(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    TRACE "MX cb: code=";code;", active="; mx->active
    cy->active = mx->active    
END SUB    

SUB plcb(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    TRACE "PL cb: code=";code;", color=";pl->color    
END SUB    

SUB cycb(BYVAL g AS GTGADGET PTR, BYVAL code AS UINTEGER)
    TRACE "CY cb: code=";code;", active="; cy->active
    mx->active=cy->active    
END SUB    

WINDOW 1, "GadTools Tutorial 6"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create a ListView gadget

'DIM AS ExecList choices = ExecList(NT_USER)

'choices.AddTail(NEW ExecNode (,,"First"))
'choices.AddTail(NEW ExecNode (,,"Second"))
'choices.AddTail(NEW ExecNode (,,"Third"))

'lv = GTGADGET (LISTVIEW_KIND,  ( 75, 20)-(235, 132), "ListView", 0, 1,_
'GTLV_ShowSelected, NULL, GTLV_Labels, @choices.l, TAG_DONE)

REM create a MX gadget

DIM AS STRING mxlabels(STATIC 3)
mxlabels(0)="First" 
mxlabels(1)="Second" 
mxlabels(2)="Third" 
mxlabels(3)=NULL 

mx = NEW GTMX ("MX", @mxlabels(0), (350, 20)-(450, 42))

REM create a palette gadget

pl = NEW GTPALETTE ("Palette", 4, (490, 20)-(550, 100))

REM create a cycle gadget

cy = NEW GTCYCLE ("Cycle", @mxlabels(0), (350, 100)-(450, 114))

GTGADGETS DEPLOY

REM message handling

'ON GTG UP   CALL lv, lvcb, NULL
mx->gadgetdown_cb = mxcb
pl->gadgetup_cb = plcb
cy->gadgetup_cb = cycb

WHILE TRUE
    SLEEP
WEND

