'
' AQB Tetris
'
' based on Colour Maximite tetris by David Murray *edited by Daedan Bamkin
' http://www.the8bitguy.com/download-davids-software/
'
' AQB port by G. Bartsch
'

OPTION EXPLICIT

RANDOMIZE TIMER

CONST AS INTEGER SCALE_X    =   9, SCALE_Y    =   9
CONST AS INTEGER GRID_XO    = 115, GRID_YO    =   6
CONST AS INTEGER PREVIEW_XO = 242, PREVIEW_YO =  37
CONST AS INTEGER LINES_XO   = 242, LINES_YO   = 100
CONST AS INTEGER SCORE_XO   = 242, SCORE_YO   = 148

DIM AS SINGLE startTime = TIMER ( )

DIM SHARED AS INTEGER grid ( 10, 20 )
DIM SHARED AS INTEGER rx ( 4, 4 ), ry ( 4, 4 )
DIM SHARED AS INTEGER piecex ( 4 )
DIM SHARED AS INTEGER piecey ( 4 )

DIM SHARED AS INTEGER PBAG ( 7 ) : REM one bag of 7 pieces

DIM SHARED AS INTEGER PP : REM preview piece
DIM SHARED AS INTEGER PC : REM current piece
DIM SHARED AS INTEGER ROT : REM rotation
DIM SHARED AS INTEGER L = 0 : REM lines
DIM SHARED AS INTEGER S = 0 : REM score
DIM SHARED AS INTEGER difficulty = 50 : REM score increase per dropped line

DIM SHARED AS INTEGER PBAGC = 7 : REM counter of the current piece in the bag, 7 means new bag

DIM SHARED AS BOOLEAN gameover = FALSE
DIM SHARED AS BOOLEAN doquit   = FALSE

SUB NEWBAG
    PBAG(0) = Int ( Rnd (1) * 7 ) + 1     
    FOR i AS INTEGER = 1 TO 6
        PBAG(i) = Int ( Rnd (1) * 7 ) + 1
        FOR k AS INTEGER = 0 TO i-1
            WHILE PBAG(i) = PBAG(k)
                PBAG(i) = Int ( Rnd (1) * 7 ) + 1
                k = 0                
            WEND 
        NEXT k            
    NEXT i        
END SUB

SUB DEFINEPIECE ( BYVAL p AS INTEGER )
    SELECT CASE p
    CASE 1
        REM BLUE J PIECE
        piecex ( 1 ) = 5 : piecey ( 1 ) = 0
        piecex ( 2 ) = 5 : piecey ( 2 ) = 1
        piecex ( 3 ) = 5 : piecey ( 3 ) = 2
        piecex ( 4 ) = 4 : piecey ( 4 ) = 2
        rx ( 1, 1 ) = -1 : ry ( 1, 1 ) = 1
        rx ( 1, 2 ) = 0 : ry ( 1, 2 ) = 0
        rx ( 1, 3 ) = 1 : ry ( 1, 3 ) = -1
        rx ( 1, 4 ) = 2 : ry ( 1, 4 ) = 0
        rx ( 2, 1 ) = 1 : ry ( 2, 1 ) = 1
        rx ( 2, 2 ) = 0 : ry ( 2, 2 ) = 0
        rx ( 2, 3 ) = -1 : ry ( 2, 3 ) = -1
        rx ( 2, 4 ) = 0 : ry ( 2, 4 ) = -2
        rx ( 3, 1 ) = 1 : ry ( 3, 1 ) = -1
        rx ( 3, 2 ) = 0 : ry ( 3, 2 ) = 0
        rx ( 3, 3 ) = -1 : ry ( 3, 3 ) = 1
        rx ( 3, 4 ) = -2 : ry ( 3, 4 ) = 0
        rx ( 4, 1 ) = -1 : ry ( 4, 1 ) = -1
        rx ( 4, 2 ) = 0 : ry ( 4, 2 ) = 0
        rx ( 4, 3 ) = 1 : ry ( 4, 3 ) = 1
        rx ( 4, 4 ) = 0 : ry ( 4, 4 ) = 2
    CASE 2
        REM GREEN S PIECE
        piecex ( 1 ) = 4 : piecey ( 1 ) = 0
        piecex ( 2 ) = 4 : piecey ( 2 ) = 1
        piecex ( 3 ) = 5 : piecey ( 3 ) = 1
        piecex ( 4 ) = 5 : piecey ( 4 ) = 2
        rx ( 1, 1 ) = -1 : ry ( 1, 1 ) = 1
        rx ( 1, 2 ) = 0 : ry ( 1, 2 ) = 0
        rx ( 1, 3 ) = -1 : ry ( 1, 3 ) = -1
        rx ( 1, 4 ) = 0 : ry ( 1, 4 ) = -2
        rx ( 2, 1 ) = 1 : ry ( 2, 1 ) = 1
        rx ( 2, 2 ) = 0 : ry ( 2, 2 ) = 0
        rx ( 2, 3 ) = -1 : ry ( 2, 3 ) = 1
        rx ( 2, 4 ) = -2 : ry ( 2, 4 ) = 0
        rx ( 3, 1 ) = 1 : ry ( 3, 1 ) = -1
        rx ( 3, 2 ) = 0 : ry ( 3, 2 ) = 0
        rx ( 3, 3 ) = 1 : ry ( 3, 3 ) = 1
        rx ( 3, 4 ) = 0 : ry ( 3, 4 ) = 2
        rx ( 4, 1 ) = -1 : ry ( 4, 1 ) = -1
        rx ( 4, 2 ) = 0 : ry ( 4, 2 ) = 0
        rx ( 4, 3 ) = 1 : ry ( 4, 3 ) = -1
        rx ( 4, 4 ) = 2 : ry ( 4, 4 ) = 0
    CASE 3
        REM CYAN L PIECE
        piecex ( 1 ) = 4 : piecey ( 1 ) = 0
        piecex ( 2 ) = 4 : piecey ( 2 ) = 1
        piecex ( 3 ) = 4 : piecey ( 3 ) = 2
        piecex ( 4 ) = 5 : piecey ( 4 ) = 2
        rx ( 1, 1 ) = -1 : ry ( 1, 1 ) = 1
        rx ( 1, 2 ) = 0 : ry ( 1, 2 ) = 0
        rx ( 1, 3 ) = 1 : ry ( 1, 3 ) = -1
        rx ( 1, 4 ) = 0 : ry ( 1, 4 ) = -2
        rx ( 2, 1 ) = 1 : ry ( 2, 1 ) = 1
        rx ( 2, 2 ) = 0 : ry ( 2, 2 ) = 0
        rx ( 2, 3 ) = -1 : ry ( 2, 3 ) = -1
        rx ( 2, 4 ) = -2 : ry ( 2, 4 ) = 0
        rx ( 3, 1 ) = 1 : ry ( 3, 1 ) = -1
        rx ( 3, 2 ) = 0 : ry ( 3, 2 ) = 0
        rx ( 3, 3 ) = -1 : ry ( 3, 3 ) = 1
        rx ( 3, 4 ) = 0 : ry ( 3, 4 ) = 2
        rx ( 4, 1 ) = -1 : ry ( 4, 1 ) = -1
        rx ( 4, 2 ) = 0 : ry ( 4, 2 ) = 0
        rx ( 4, 3 ) = 1 : ry ( 4, 3 ) = 1
        rx ( 4, 4 ) = 2 : ry ( 4, 4 ) = 0
    CASE 4
        REM RED Z PIECE
        piecex ( 1 ) = 5 : piecey ( 1 ) = 0
        piecex ( 2 ) = 5 : piecey ( 2 ) = 1
        piecex ( 3 ) = 4 : piecey ( 3 ) = 1
        piecex ( 4 ) = 4 : piecey ( 4 ) = 2
        rx ( 1, 1 ) = -1 : ry ( 1, 1 ) = 1
        rx ( 1, 2 ) = 0 : ry ( 1, 2 ) = 0
        rx ( 1, 3 ) = 1 : ry ( 1, 3 ) = 1
        rx ( 1, 4 ) = 2 : ry ( 1, 4 ) = 0
        rx ( 2, 1 ) = 1 : ry ( 2, 1 ) = -1
        rx ( 2, 2 ) = 0 : ry ( 2, 2 ) = 0
        rx ( 2, 3 ) = -1 : ry ( 2, 3 ) = -1
        rx ( 2, 4 ) = -2 : ry ( 2, 4 ) = 0
        rx ( 3, 1 ) = -1 : ry ( 3, 1 ) = 1
        rx ( 3, 2 ) = 0 : ry ( 3, 2 ) = 0
        rx ( 3, 3 ) = 1 : ry ( 3, 3 ) = 1
        rx ( 3, 4 ) = 2 : ry ( 3, 4 ) = 0
        rx ( 4, 1 ) = 1 : ry ( 4, 1 ) = -1
        rx ( 4, 2 ) = 0 : ry ( 4, 2 ) = 0
        rx ( 4, 3 ) = -1 : ry ( 4, 3 ) = -1
        rx ( 4, 4 ) = -2 : ry ( 4, 4 ) = 0
    CASE 5
        REM PURPLE T PIECE
        piecex ( 1 ) = 5 : piecey ( 1 ) = 0
        piecex ( 2 ) = 4 : piecey ( 2 ) = 1
        piecex ( 3 ) = 5 : piecey ( 3 ) = 1
        piecex ( 4 ) = 6 : piecey ( 4 ) = 1
        rx ( 1, 1 ) = -1 : ry ( 1, 1 ) = 1
        rx ( 1, 2 ) = 1 : ry ( 1, 2 ) = 1
        rx ( 1, 3 ) = 0 : ry ( 1, 3 ) = 0
        rx ( 1, 4 ) = -1 : ry ( 1, 4 ) = -1
        rx ( 2, 1 ) = 1 : ry ( 2, 1 ) = 1
        rx ( 2, 2 ) = 1 : ry ( 2, 2 ) = -1
        rx ( 2, 3 ) = 0 : ry ( 2, 3 ) = 0
        rx ( 2, 4 ) = -1 : ry ( 2, 4 ) = 1
        rx ( 3, 1 ) = 1 : ry ( 3, 1 ) = -1
        rx ( 3, 2 ) = -1 : ry ( 3, 2 ) = -1
        rx ( 3, 3 ) = 0 : ry ( 3, 3 ) = 0
        rx ( 3, 4 ) = 1 : ry ( 3, 4 ) = 1
        rx ( 4, 1 ) = -1 : ry ( 4, 1 ) = -1
        rx ( 4, 2 ) = -1 : ry ( 4, 2 ) = 1
        rx ( 4, 3 ) = 0 : ry ( 4, 3 ) = 0
        rx ( 4, 4 ) = 1 : ry ( 4, 4 ) = -1
    CASE 6
        REM YELLOW O PIECE
        piecex ( 1 ) = 4 : piecey ( 1 ) = 0
        piecex ( 2 ) = 5 : piecey ( 2 ) = 0
        piecex ( 3 ) = 4 : piecey ( 3 ) = 1
        piecex ( 4 ) = 5 : piecey ( 4 ) = 1
        rx ( 1, 1 ) = 0 : ry ( 1, 1 ) = 0
        rx ( 1, 2 ) = 0 : ry ( 1, 2 ) = 0
        rx ( 1, 3 ) = 0 : ry ( 1, 3 ) = 0
        rx ( 1, 4 ) = 0 : ry ( 1, 4 ) = 0
        rx ( 2, 1 ) = 0 : ry ( 2, 1 ) = 0
        rx ( 2, 2 ) = 0 : ry ( 2, 2 ) = 0
        rx ( 2, 3 ) = 0 : ry ( 2, 3 ) = 0
        rx ( 2, 4 ) = 0 : ry ( 2, 4 ) = 0
        rx ( 3, 1 ) = 0 : ry ( 3, 1 ) = 0
        rx ( 3, 2 ) = 0 : ry ( 3, 2 ) = 0
        rx ( 3, 3 ) = 0 : ry ( 3, 3 ) = 0
        rx ( 3, 4 ) = 0 : ry ( 3, 4 ) = 0
        rx ( 4, 1 ) = 0 : ry ( 4, 1 ) = 0
        rx ( 4, 2 ) = 0 : ry ( 4, 2 ) = 0
        rx ( 4, 3 ) = 0 : ry ( 4, 3 ) = 0
        rx ( 4, 4 ) = 0 : ry ( 4, 4 ) = 0
    CASE 7
        REM WHITE I PIECE
        piecex ( 1 ) = 5 : piecey ( 1 ) = 0
        piecex ( 2 ) = 5 : piecey ( 2 ) = 1
        piecex ( 3 ) = 5 : piecey ( 3 ) = 2
        piecex ( 4 ) = 5 : piecey ( 4 ) = 3
        rx ( 1, 1 ) = -1 : ry ( 1, 1 ) = 1
        rx ( 1, 2 ) = 0 : ry ( 1, 2 ) = 0
        rx ( 1, 3 ) = 1 : ry ( 1, 3 ) = -1
        rx ( 1, 4 ) = 2 : ry ( 1, 4 ) = -2
        rx ( 2, 1 ) = 1 : ry ( 2, 1 ) = -1
        rx ( 2, 2 ) = 0 : ry ( 2, 2 ) = 0
        rx ( 2, 3 ) = -1 : ry ( 2, 3 ) = 1
        rx ( 2, 4 ) = -2 : ry ( 2, 4 ) = 2
        rx ( 3, 1 ) = -1 : ry ( 3, 1 ) = 1
        rx ( 3, 2 ) = 0 : ry ( 3, 2 ) = 0
        rx ( 3, 3 ) = 1 : ry ( 3, 3 ) = -1
        rx ( 3, 4 ) = 2 : ry ( 3, 4 ) = -2
        rx ( 4, 1 ) = 1 : ry ( 4, 1 ) = -1
        rx ( 4, 2 ) = 0 : ry ( 4, 2 ) = 0
        rx ( 4, 3 ) = -1 : ry ( 4, 3 ) = 1
        rx ( 4, 4 ) = -2 : ry ( 4, 4 ) = 2
    END SELECT
END SUB

SUB PREVIEW
    IF PBAGC = 7 THEN
        NEWBAG
        PBAGC = 0
        PP = PBAG ( PBAGC )
        PBAGC = PBAGC + 1        
    ELSE
        PP = PBAG ( PBAGC )
        PBAGC = PBAGC + 1        
    END IF
    
    DEFINEPIECE PP
    
    LINE ( PREVIEW_XO, PREVIEW_YO -1 ) - ( PREVIEW_XO + 4 * SCALE_X, PREVIEW_YO + 4 * SCALE_Y -1 ), 0, BF
    FOR p AS INTEGER = 1 TO 4
        DIM AS INTEGER X = piecex ( p ), Y = piecey ( p )
        DIM AS INTEGER XT = ( X -3 ) * SCALE_X + PREVIEW_XO, YT = Y * SCALE_Y + PREVIEW_YO
        LINE ( XT, YT ) - ( XT + SCALE_X -2, YT + SCALE_Y -2 ), PP, BF
        LINE ( XT + 1, YT + 1 ) - ( XT + SCALE_X -3, YT + 1 ), 0
        LINE ( XT + SCALE_X -3, YT + 1 ) - ( XT + SCALE_X -3, YT + SCALE_Y -3 ), 0
    NEXT p
END SUB

SUB CLEARSCREENGRID
    LINE ( GRID_XO, GRID_YO ) - ( GRID_XO + 10 * SCALE_X, GRID_YO + 20 * SCALE_Y ), 0, BF
END SUB

SUB REDRAWSCREENGRID
    FOR Y AS INTEGER = 0 TO 19
        FOR X AS INTEGER = 0 TO 9
            DIM AS INTEGER XT = X * SCALE_X + GRID_XO, YT = Y * SCALE_Y + GRID_YO
            LINE ( XT, YT ) - ( XT + SCALE_X -2, YT + SCALE_Y -2 ), grid ( X, Y ), BF
            LINE ( XT + 1, YT + 1 ) - ( XT + SCALE_X -3, YT + 1 ), 0
            LINE ( XT + SCALE_X -3, YT + 1 ) - ( XT + SCALE_X -3, YT + SCALE_Y -3 ), 0
        NEXT X
    NEXT Y
END SUB

SUB SHOWSCORE
    LINE ( LINES_XO, LINES_YO ) - ( LINES_XO + 6 * SCALE_X, LINES_YO + 2 * SCALE_Y ), 0, BF
    LINE ( SCORE_XO, SCORE_YO ) - ( SCORE_XO + 6 * SCALE_X, SCORE_YO + 2 * SCALE_Y ), 0, BF
    COLOR 7, 0
    LOCATE XY (256, 160) : PRINT S;
    LOCATE XY (256, 112) : PRINT L;    
END SUB    

SUB DROPGRID ( BYREF CY AS INTEGER )
    FOR X AS INTEGER = 0 TO 9
        FOR YY AS INTEGER = CY TO 1 STEP -1
            grid ( X, YY ) = grid ( X, YY -1 )
        NEXT YY
    NEXT X
    L = L + 1
    S = S + difficulty
    SHOWSCORE    
    CLEARSCREENGRID
    REDRAWSCREENGRID
END SUB

SUB CPRINT ( BYVAL Y AS INTEGER, S AS STRING )
    
    DIM AS INTEGER X = 160 - TEXTWIDTH ( S ) / 2
    LOCATE XY (X, Y)
    PRINT S;
    
END SUB

SUB GAMEOVERMAN
    
    LINE (80, 60)-(240, 170),5,B
    LINE (81, 61)-(239, 169),0,BF    
    
    CPRINT  77, "                 "
    CPRINT  88, "    GAME OVER    "
    CPRINT  99, "    ---------    "
    CPRINT 110, "                 "
    CPRINT 121, " WOULD YOU LIKE  "
    CPRINT 132, " TO PLAY ANOTHER "
    CPRINT 143, "    GAME Y/N?    "
    CPRINT 154, "                 "
    
    WHILE NOT gameover
        DIM AS STRING key = INKEY$
        IF key = "" THEN
            SLEEP
        ELSE
            gameover = TRUE
            IF ( key = "n" ) OR ( key = "N" ) THEN
                doquit = TRUE
            END IF
        END IF
    WEND
END SUB

SUB CHECKGRID
    FOR CY AS INTEGER = 0 TO 19
        DIM AS INTEGER ROW = 0
        FOR X AS INTEGER = 0 TO 9
            IF grid ( X, CY ) <> 0 THEN ROW = ROW + 1
        NEXT X
        IF ROW = 10 THEN DROPGRID ( CY )
        IF CY = 0 AND ROW > 0 THEN GAMEOVERMAN
    NEXT CY
END SUB

SUB NEWPIECE
    CHECKGRID
    ROT = 1
    PC = PP
    PREVIEW
    DEFINEPIECE PC
END SUB

SUB DRAWPIECE
    FOR p AS INTEGER = 1 TO 4
        DIM AS INTEGER X = piecex ( p ), Y = piecey ( p )
        grid ( X, Y ) = PC
        DIM AS INTEGER XT = X * SCALE_X + GRID_XO, YT = Y * SCALE_Y + GRID_YO
        LINE ( XT, YT ) - ( XT + SCALE_X -2, YT + SCALE_Y -2 ), PC, BF
        LINE ( XT + 1, YT + 1 ) - ( XT + SCALE_X -3, YT + 1 ), 0
        LINE ( XT + SCALE_X -3, YT + 1 ) - ( XT + SCALE_X -3, YT + SCALE_Y -3 ), 0
    NEXT p
END SUB

SUB ERASEPIECE
    FOR p AS INTEGER = 1 TO 4
        DIM AS INTEGER X = piecex ( p ), Y = piecey ( p )
        grid ( X, Y ) = 0
        DIM AS INTEGER XT = X * SCALE_X + GRID_XO, YT = Y * SCALE_Y + GRID_YO
        LINE ( XT, YT ) - ( XT + SCALE_X -2, YT + SCALE_Y -2 ), 0, BF
    NEXT p
END SUB

FUNCTION MOVEDOWN AS BOOLEAN
    REM ERASEPIECE
    DIM AS BOOLEAN E = FALSE
    ' bottom reached ?
    FOR p AS INTEGER = 1 TO 4
        IF piecey ( p ) = 19 THEN E = TRUE
    NEXT p
    IF E THEN DRAWPIECE : NEWPIECE : RETURN TRUE
    ' collision ?
    FOR p AS INTEGER = 1 TO 4
        IF grid ( piecex ( p ), piecey ( p ) + 1 ) <> 0 THEN E = TRUE
    NEXT p
    IF E THEN DRAWPIECE : NEWPIECE : RETURN TRUE
    FOR p AS INTEGER = 1 TO 4 : piecey ( p ) = piecey ( p ) + 1 : NEXT p
    REM DRAWPIECE
    ' IF A=90 or A=122 THEN MOVEDOWN
    ' A=0
    RETURN FALSE
END FUNCTION

SUB timer_movedown
    ERASEPIECE
    MOVEDOWN
    DRAWPIECE
END SUB

SUB MOVERIGHT
    ERASEPIECE
    DIM AS BOOLEAN E = FALSE
    FOR p AS INTEGER = 1 TO 4
        IF piecex ( p ) = 9 THEN E = TRUE
    NEXT p
    IF E THEN DRAWPIECE : RETURN
    FOR p AS INTEGER = 1 TO 4
        IF grid ( piecex ( p ) + 1, piecey ( p ) ) <> 0 THEN E = TRUE
    NEXT p
    IF E THEN DRAWPIECE : RETURN
    FOR p AS INTEGER = 1 TO 4 : piecex ( p ) = piecex ( p ) + 1 : NEXT p
    DRAWPIECE
END SUB

SUB MOVELEFT
    ERASEPIECE
    DIM AS BOOLEAN E = FALSE
    FOR p AS INTEGER = 1 TO 4
        IF piecex ( p ) = 0 THEN E = TRUE
    NEXT p
    IF E THEN DRAWPIECE : RETURN
    FOR p AS INTEGER = 1 TO 4
        IF grid ( piecex ( p ) -1, piecey ( p ) ) <> 0 THEN E = TRUE
    NEXT p
    IF E THEN DRAWPIECE : RETURN
    FOR p AS INTEGER = 1 TO 4
        piecex ( p ) = piecex ( p ) -1
    NEXT p
    DRAWPIECE
    
END SUB

SUB LEAVEPIECE
    ERASEPIECE
    WHILE NOT MOVEDOWN : WEND
    REM DRAWPIECE
END SUB

SUB ROTATEPIECE
    REM is there enough room to rotate?
    FOR p AS INTEGER = 1 TO 4
        DIM AS INTEGER X = piecex ( p ) + rx ( ROT, p )
        REM TRACE "ROTATEPIECE X=";X        
        IF ( X < 0 ) OR ( X > 9 ) THEN
            RETURN
        END IF
    NEXT p
    ERASEPIECE
    FOR p AS INTEGER = 1 TO 4
        piecex ( p ) = piecex ( p ) + rx ( ROT, p )
        piecey ( p ) = piecey ( p ) + ry ( ROT, p )
    NEXT p
    ROT = ROT + 1 : IF ROT = 5 THEN ROT = 1
    DRAWPIECE
END SUB

SUB ENDGAME
    
    TIMER OFF 1
    
    COLOR 7
    
    CPRINT 64, " QUITTING? "
    CPRINT 75, " ARE YOU "
    CPRINT 86, " SURE Y/N? "
    
    WHILE TRUE
        
        DIM AS STRING key = INKEY$
        
        IF key = "" THEN
            SLEEP
        ELSE
            
            IF ( key = "y" ) OR ( key = "Y" ) THEN
                gameover = TRUE
            ELSE
                CLEARSCREENGRID
                REDRAWSCREENGRID
                TIMER ON 1
            END IF
            
            RETURN
            
        END IF
    WEND
    
END SUB

SCREEN 2, 320, 200, 3, 0, "AQB Tetris"
WINDOW 4,,, AW_FLAG_BACKDROP OR AW_FLAG_BORDERLESS OR AW_FLAG_ACTIVATE OR AW_FLAG_GIMMEZEROZERO, 2

PALETTE 0, 0, 0, 0 : REM black
PALETTE 1, 0, 0, 1 : REM blue
PALETTE 2, 0, 1, 0 : REM green
PALETTE 3, 0, 1, 1 : REM cyan
PALETTE 4, 1, 0, 0 : REM red
PALETTE 5, 1, 0, 1 : REM purple
PALETTE 6, 1, 1, 0 : REM yellow
PALETTE 7, 1, 1, 1 : REM white

DIM AS FONT_t PTR fontHeadline = FONT("cubberly.font", 32, "PROGDIR:Fonts")
DIM AS FONT_t PTR fontText     = FONT("2001.font",      8, "PROGDIR:Fonts")

WHILE NOT doquit
    
    CLS
    
    COLOR 7 : FONT fontText
    
    CPRINT 22, "Welcome to"
    FONT fontHeadline    
    CPRINT 44, "AQB TETRIS"
    FONT fontText 
    CPRINT 66, "Based on COLOUR MAXIMITE TETRIS"
    CPRINT 88, "By David Murray"
    CPRINT 99, "AQB port by Guenter Bartsch"
    
    LOCATE XY (15, 128) : PRINT "Please enter level 1-10";
    LOCATE XY (15, 139) : PRINT "default being 2, 10 being ridiculous";
    LOCATE XY (15, 150) : PRINT "Good Luck!";
    LINE ( 2, 116 ) - ( 317, 156 ), 5, B
    LINE ( 0, 114 ) - ( 319, 158 ), 5, B
    
    DIM AS SINGLE t2 = 0
    
    WHILE t2 = 0
        
        DIM AS STRING key = INKEY$
        
        IF key = "" THEN
            
            SLEEP
            
        ELSE
            
            SELECT CASE ASC ( key )
            CASE 48 : t2 = 50 : difficulty = 500 : REM key 0
            CASE 49 : t2 = 500 : difficulty = 50 : REM key 1
            CASE 50, 13 : t2 = 450 : difficulty = 100 : REM key 2 or enter
            CASE 51 : t2 = 400 : difficulty = 150 : REM key 3
            CASE 52 : t2 = 350 : difficulty = 200 : REM key 4
            CASE 53 : t2 = 300 : difficulty = 250 : REM key 5
            CASE 54 : t2 = 250 : difficulty = 300 : REM key 6
            CASE 55 : t2 = 200 : difficulty = 350 : REM key 7
            CASE 56 : t2 = 150 : difficulty = 400 : REM key 8
            CASE 57 : t2 = 100 : difficulty = 450 : REM key 9
            CASE 27 : GOTO quitlabel
            END SELECT
            
            ' FIXME: IF Timer>=350 Then GoSub PREVIEW:Timer=0
            
        END IF
    WEND
    
    
    '
    ' new game
    '
    
    FOR X AS INTEGER = 0 TO 9
        FOR Y AS INTEGER = 0 TO 19
            grid ( X, Y ) = 0
        NEXT Y
    NEXT X
    
    COLOR 7, 1
    CLS
    
    LOCATE XY (10,  33) : PRINT "LEFT/RIGHT";
    LOCATE XY (10,  44) : PRINT "MOVE";
    LOCATE XY (10,  66) : PRINT "UP";
    LOCATE XY (10,  77) : PRINT "ROTATE";
    LOCATE XY (10,  99) : PRINT "DOWN";
    LOCATE XY (10, 110) : PRINT "DROP";
    LOCATE XY (10, 132) : PRINT "ESC";
    LOCATE XY (10, 143) : PRINT "PAUSE/EXIT";
    
    LOCATE XY (248, 32) : PRINT "NEXT";
    LINE ( PREVIEW_XO -1, PREVIEW_YO -2 ) - ( PREVIEW_XO + 4 * SCALE_X + 1, PREVIEW_YO + 4 * SCALE_Y ), 7, B
    
    LINE ( GRID_XO -1, GRID_YO -1 ) - ( GRID_XO + 10 * SCALE_X + 1, GRID_YO + 20 * SCALE_Y + 1 ), 7, B
    LINE ( GRID_XO, GRID_YO ) - ( GRID_XO + 10 * SCALE_X, GRID_YO + 20 * SCALE_Y ), 0, BF
    
    LOCATE XY (248, 96) : PRINT "LINES"
    LINE ( LINES_XO -1, LINES_YO -1 ) - ( LINES_XO + 6 * SCALE_X + 1, LINES_YO + 2 * SCALE_Y + 1 ), 7, B
    LINE ( LINES_XO, LINES_YO ) - ( LINES_XO + 6 * SCALE_X, LINES_YO + 2 * SCALE_Y ), 0, BF
    
    LOCATE XY (248, 144) : PRINT "SCORE"
    LINE ( SCORE_XO -1, SCORE_YO -1 ) - ( SCORE_XO + 6 * SCALE_X + 1, SCORE_YO + 2 * SCALE_Y + 1 ), 7, B
    LINE ( SCORE_XO, SCORE_YO ) - ( SCORE_XO + 6 * SCALE_X, SCORE_YO + 2 * SCALE_Y ), 0, BF
    
    SHOWSCORE    
    
    PREVIEW
    NEWPIECE
    DRAWPIECE
    
    DIM delay AS SINGLE = t2 / 1000
    
    ON TIMER CALL 1, delay, timer_movedown
    TIMER ON 1
    
    REDRAWSCREENGRID
    
    gameover = FALSE
    
    WHILE NOT gameover
        SLEEP
        DIM AS STRING k = INKEY$
        IF k = "" THEN CONTINUE
        SELECT CASE ASC ( k )
        CASE 27 : ' Escape
            ENDGAME
        CASE 29 : ' cursor down
            LEAVEPIECE
        CASE 28 :  : REM cursor up
            ROTATEPIECE
        CASE 30 : ' cursor right
            MOVERIGHT
        CASE 31 : ' cursor left
            MOVELEFT
        END SELECT
    WEND
    
    TIMER OFF 1
    
WEND

quitlabel : 

