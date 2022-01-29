REM TAGLIST / TAGITEM tutorial

OPTION EXPLICIT

IMPORT OSUtility

SUB traceTagList (BYVAL tagList AS TagItem PTR)
    
    REM iterate over tagitems
    
    DIM AS TagItem PTR tstate = tagList
    
    DIM AS INTEGER cnt=0
    TRACE "[";    
    DO
        
        DIM AS TagItem PTR t = NextTagItem (@tstate)
        
        IF t=NULL THEN EXIT
        
        TRACE " (";t->ti_Tag;":";t->ti_Data;")";
        cnt = cnt + 1    
    LOOP    
    
    TRACE " ] #";cnt
    
END SUB    

REM create a TagItem list containing 3 TagItems + end marker

DIM AS TagItem PTR mytagitems = TAGITEMS (42, 1000, 43, 500, 44, 2000, TAG_DONE)

TRACE "whole tag list: ";
traceTagList mytagitems

REM find a tag item

DIM AS TagItem PTR myti = FindTagItem (43, mytagitems)

IF myti<>NULL THEN 
    TRACE "found tag item: tag=";myti->ti_Tag;", data=";myti->ti_Data 
ELSE 
    TRACE "tag item not found." 
END IF


REM filter TagItems

FilterTagItems (mytagitems, TAGS(43,44), TAGFILTER_AND)
TRACE "filtered tag list: ";
traceTagList mytagitems


