REM 2 voice music demo

OPTION EXPLICIT
IMPORT IFFSupport

CONST AS INTEGER NUM_NOTES    = 21
CONST AS INTEGER SCORE_LEN    = 32
CONST AS INTEGER SCORE_VOICES = 2
CONST AS SINGLE  TEMPO        = 0.15

DIM SHARED AS BITMAP_t PTR clefbm=NULL, notesbm=NULL

DIM SHARED AS STRING       noteNames     (NUM_NOTES-1)
DIM SHARED AS SINGLE       noteFreqs     (NUM_NOTES-1)

DIM SHARED AS INTEGER      scoreNote     (SCORE_VOICES-1, SCORE_LEN-1)
DIM SHARED AS INTEGER      scoreDuration (SCORE_VOICES-1, SCORE_LEN-1)

SUB initNotes
    FOR i AS INTEGER = 0 TO NUM_NOTES-1
        READ noteNames(i), noteFreqs(i)
        REM TRACE noteNames(i), noteFreqs(i)        
    NEXT i 
END SUB    

FUNCTION noteLookup(BYVAL note AS STRING) AS SINGLE
    FOR i AS INTEGER = 0 TO NUM_NOTES-1
        IF noteNames(i)=note THEN RETURN i
    NEXT i 
    RETURN -1     
END FUNCTION    

SUB initScore 
    FOR i AS INTEGER = 0 TO SCORE_LEN-1
        FOR v AS integer = 0 TO SCORE_VOICES-1        
            DIM AS STRING note
            DIM AS INTEGER l, n
            READ note, l
            n = noteLookup(note)
            REM TRACE note, l, n
            scoreNote(v, i) = n 
            scoreDuration(v, i) = l
        NEXT v            
    NEXT i        
END SUB    

SUB drawNote (n AS INTEGER, duration AS INTEGER, x AS INTEGER)
    
    DIM AS integer idx    
    
    SELECT CASE duration
    CASE 2:
        idx=4
    CASE 3:
        idx=3
    CASE 1:
        idx=6        
    CASE 4:
        idx=2
    CASE 8:
        idx=0        
    CASE ELSE
        idx=7        
    END SELECT        
    
    PUT (x, 76-n*5/2), notesbm, &HE0, (idx*32+8,0)-(idx*32+22,16)    
    
END SUB    

SUB drawScore 
    
    LINE (5,10)-(605,100),2,BF
    
    PUT (10, 20), clefbm, &HC0
    
    FOR i AS INTEGER = 0 TO 4
        LINE (30,30+i*5)-(600,30+i*5),1
        LINE (30,60+i*5)-(600,60+i*5),1    
    NEXT i    
    
    FOR i AS INTEGER = 0 TO SCORE_LEN-1
        FOR v AS INTEGER = 0 TO SCORE_VOICES-1
            IF scoreNote(v,i) >= 0 THEN        
                drawNote scoreNote(v,i), scoreDuration(v,i), 80+i*16
            END IF            
        NEXT v        
    NEXT i
    
END SUB    

SUB playSong
    FOR i AS INTEGER = 0 TO SCORE_LEN-1
        
        FOR v AS INTEGER = 0 TO SCORE_VOICES-1    
            
            REM TRACE i, v        
            
            DIM n AS INTEGER = scoreNote(v,i)
            IF n<0 THEN CONTINUE
            
            DIM f AS SINGLE = noteFreqs(n)
            DIM AS INTEGER d = scoreDuration(v, i) 
            
            REM TRACE "f=";f;", d=";d;", v=";v    
            
            SOUND f, TEMPO*d, 127, v
            
            REM SOUND STOP    
            
            REM SOUND WAIT
        NEXT v
        
        LINE (64+i*16,90)-(64+i*16+15,90),2
        LINE (80+i*16,90)-(80+i*16+15,90),3
        
        SLEEP FOR TEMPO
    NEXT i
    SOUND WAIT
END SUB

initNotes
initScore

WINDOW 1, "Music Demo"

ILBM LOAD BITMAP "PROGDIR:imgs/clef.ilbm", clefbm
ILBM LOAD BITMAP "PROGDIR:imgs/notes.ilbm", notesbm

BITMAP MASK notesbm

REM load .8svx instrument

DIM AS WAVE_t PTR w=NULL
IFF8SVX LOAD WAVE "PROGDIR:8svx/Piano.8svx", w

WAVE 0, w
WAVE 1, w

DIM AS BOOLEAN finished=FALSE

DO
    
    drawScore 
    playSong
    
    LOCATE 20, 1 : PRINT "PRESS: P TO PLAY, Q TO QUIT";
    
    DIM AS STRING answer = ""    
    WHILE answer<>"p" AND answer<>"q"
        answer=INKEY$
        SLEEP
    WEND
    finished = answer="q"
    
LOOP UNTIL finished

REM note, frequency
DATA C2, 65.41, D2, 73.42, E2, 82.41, F2, 87.31, G2, 98!, A2, 110!, B2, 123.47
DATA C3, 130.81, D3, 146.83, E3, 164.81, F3, 174.61, G3, 196!, A3, 220!, B3, 146.94
DATA C4, 261.63, D4, 293.66, E4, 329.63, F4, 349.23, G4, 392!, A4, 440!, B4, 493.88

REM 1     -> 8
REM 1/2   -> 4
REM 1/4 . -> 3
REM 1/4   -> 2
REM 1/8   -> 1

DATA E4, 2, C3, 8
DATA N , 0, N , 0
DATA E4, 2, N , 0
DATA N , 0, N , 0
DATA F4, 2, N , 0
DATA N , 0, N , 0 
DATA G4, 2, N , 0
DATA N , 0, N , 0

DATA G4, 2, G2, 8
DATA N , 0, N , 0
DATA F4, 2, N , 0
DATA N , 0, N , 0
DATA E4, 2, N , 0
DATA N , 0, N , 0
DATA D4, 2, N , 0
DATA N , 0, N , 0

DATA C4, 2, C3, 8
DATA N , 0, N , 0
DATA C4, 2, N , 0
DATA N , 0, N , 0
DATA D4, 2, N , 0
DATA N , 0, N , 0
DATA E4, 2, N , 0
DATA N , 0, N , 0


DATA E4, 3, G2, 8
DATA N , 0, N , 0
DATA N , 0, N , 0
DATA D4, 1, N , 0
DATA D4, 4, N , 0
DATA N , 0, N , 0
DATA N , 0, N , 0
DATA N , 0, N , 0

