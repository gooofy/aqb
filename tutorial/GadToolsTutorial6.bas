REM GadTools Tutorial 6: ListView, MX, Palette, Cycle

OPTION EXPLICIT

IMPORT OSIntuition
IMPORT GadToolsSupport

DIM SHARED AS GTGADGET_t PTR lv, mx, pl, cy

REM callbacks

SUB winCloseCB (BYVAL wid AS INTEGER, BYVAL ud AS VOID PTR)
    SYSTEM
END SUB    

SUB lvcb(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    TRACE "LV cb: wid=";wid;", gid=";gid;", code=";code    
END SUB    

SUB mxcb(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    TRACE "MX cb: wid=";wid;", gid=";gid;", code=";code    
END SUB    

SUB plcb(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    TRACE "PL cb: wid=";wid;", gid=";gid;", code=";code    
END SUB    

SUB cycb(BYVAL wid AS INTEGER, BYVAL gid AS INTEGER, BYVAL code AS UINTEGER, BYVAL ud AS VOID PTR)
    TRACE "CY cb: wid=";wid;", gid=";gid;", code=";code    
END SUB    

WINDOW 1, "GadTools Tutorial 6"
ON WINDOW CLOSE CALL 1, winCloseCB

REM create a ListView gadget

DIM AS ExecList choices = ExecList(NT_USER)

choices.AddTail(NEW ExecNode (,,"First"))
choices.AddTail(NEW ExecNode (,,"Second"))
choices.AddTail(NEW ExecNode (,,"Third"))

lv = GTGADGET (LISTVIEW_KIND,  ( 75, 20)-(235, 132), "ListView", 0, 1,_
GTLV_ShowSelected, NULL, GTLV_Labels, @choices.l, TAG_DONE)

REM create a MX gadget

DIM AS STRING mxlabels(STATIC 3)
mxlabels(0)="First" 
mxlabels(1)="Second" 
mxlabels(2)="Third" 
mxlabels(3)=NULL 

mx = GTGADGET (MX_KIND, (350, 20)-(450, 42), "MX", 0, 2,_
GTMX_Labels, @mxlabels(0), GTMX_Active, 1, GTMX_Spacing, 3, TAG_DONE)

REM create a palette gadget

pl = GTGADGET (PALETTE_KIND, (490, 20)-(550, 100), "Palette", 0, 3,_
GTPA_Depth, 2, TAG_DONE)

REM create a cycle gadget

cy = GTGADGET (CYCLE_KIND, (350, 100)-(450, 114), "Cycle", 0, 4,_
GTCY_Labels, @mxlabels(0), TAG_DONE)


GTGADGETS DEPLOY

REM message handling

ON GTG UP   CALL lv, lvcb, NULL
ON GTG DOWN CALL mx, mxcb, NULL
ON GTG UP   CALL pl, plcb, NULL
ON GTG UP   CALL cy, cycb, NULL

WHILE TRUE
    SLEEP
WEND

