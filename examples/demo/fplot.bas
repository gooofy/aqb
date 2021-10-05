' function parser in AQB

OPTION EXPLICIT

DIM SHARED AS string fs = "x*x"
DIM SHARED AS boolean eol = false
DIM SHARED AS integer scPos = 0
DIM SHARED AS UBYTE scCurC
DIM SHARED AS integer scLen
DIM SHARED AS integer scSym

CONST AS integer SYM_ASTERISK = 1
CONST AS integer SYM_IDENTIFIER = 2


SUB getch
    
    eol = scPos >= scLen
    
    IF eol THEN RETURN
    
    scCurC = fs [ scPos ]
    scPos = scPos + 1
    
    
END SUB

FUNCTION IsWhitespace ( c AS UBYTE )
    
    IF c = 32 OR c = 7 THEN
        RETURN TRUE
    END IF
    RETURN false
END FUNCTION

SUB getsym
    
    WHILE NOT eol AND IsWhitespace ( scCurC )
        getch
    WEND
    
    SELECT CASE scCurC
    CASE 42 : REM *
        scSym = SYM_ASTERISK
    CASE ELSE
        IF IsVarChar ( scCurC ) THEN
            scIdentifier
        ELSE
            ERROR 1
        END IF
        
    END SELECT
    
END SUB

SUB scInit
    
    scPos = 0
    eol = false
    
    scLen = LEN ( fs )
    getch
    getsym
    
END SUB

scInit

