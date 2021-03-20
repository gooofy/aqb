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

CONST AS INTEGER SCALE_X    =   9, SCALE_Y    =  9
CONST AS INTEGER GRID_XO    = 115, GRID_YO    =  6
CONST AS INTEGER PREVIEW_XO = 242, PREVIEW_YO = 37
CONST AS INTEGER LINES_XO   = 242, LINES_YO   =100
CONST AS INTEGER SCORE_XO   = 242, SCORE_YO   =148

DIM AS SINGLE startTime = TIMER()
'PRINT "startTime=";startTime

DIM SHARED AS INTEGER grid(10,20)
DIM SHARED AS INTEGER rx(4,4), ry(4,4)
DIM SHARED AS INTEGER piecex(4)
DIM SHARED AS INTEGER piecey(4)

DIM SHARED AS INTEGER PP  : REM preview piece
DIM SHARED AS INTEGER PC  : REM current piece
DIM SHARED AS INTEGER ROT : REM rotation
DIM SHARED AS INTEGER L=0 : REM lines
DIM SHARED AS INTEGER S=0 : REM score

SUB DEFINEPIECE (BYVAL p AS INTEGER)
    ' LOCATE 7,1 : PRINT "DEFINEPIECE: p=";p
    SELECT CASE p
        CASE 1
            REM BLUE J PIECE
            ' LOCATE 5,1 : PRINT "DEFINEPIECE: BLUE J PIECE"
            piecex(1)=5:piecey(1)=0
            piecex(2)=5:piecey(2)=1
            piecex(3)=5:piecey(3)=2
            piecex(4)=4:piecey(4)=2
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=1:ry(1,3)=-1
            rx(1,4)=2:ry(1,4)=0
            rx(2,1)=1:ry(2,1)=1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=-1
            rx(2,4)=0:ry(2,4)=-2
            rx(3,1)=1:ry(3,1)=-1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=-1:ry(3,3)=1
            rx(3,4)=-2:ry(3,4)=0
            rx(4,1)=-1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=1:ry(4,3)=1
            rx(4,4)=0:ry(4,4)=2
        CASE 2
            REM GREEN S PIECE
            piecex(1)=4:piecey(1)=0
            piecex(2)=4:piecey(2)=1
            piecex(3)=5:piecey(3)=1
            piecex(4)=5:piecey(4)=2
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=-1:ry(1,3)=-1
            rx(1,4)=0:ry(1,4)=-2
            rx(2,1)=1:ry(2,1)=1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=1
            rx(2,4)=-2:ry(2,4)=0
            rx(3,1)=1:ry(3,1)=-1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=1:ry(3,3)=1
            rx(3,4)=0:ry(3,4)=2
            rx(4,1)=-1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=1:ry(4,3)=-1
            rx(4,4)=2:ry(4,4)=0
        CASE 3
            REM CYAN L PIECE
            piecex(1)=4:piecey(1)=0
            piecex(2)=4:piecey(2)=1
            piecex(3)=4:piecey(3)=2
            piecex(4)=5:piecey(4)=2
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=1:ry(1,3)=-1
            rx(1,4)=0:ry(1,4)=-2
            rx(2,1)=1:ry(2,1)=1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=-1
            rx(2,4)=-2:ry(2,4)=0
            rx(3,1)=1:ry(3,1)=-1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=-1:ry(3,3)=1
            rx(3,4)=0:ry(3,4)=2
            rx(4,1)=-1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=1:ry(4,3)=1
            rx(4,4)=2:ry(4,4)=0
        CASE 4
            REM RED Z PIECE
            ' LOCATE 5,1 : PRINT "DEFINEPIECE: RED Z PIECE"
            piecex(1)=5:piecey(1)=0
            piecex(2)=5:piecey(2)=1
            piecex(3)=4:piecey(3)=1
            piecex(4)=4:piecey(4)=2
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=1:ry(1,3)=1
            rx(1,4)=2:ry(1,4)=0
            rx(2,1)=1:ry(2,1)=-1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=-1
            rx(2,4)=-2:ry(2,4)=0
            rx(3,1)=-1:ry(3,1)=1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=1:ry(3,3)=1
            rx(3,4)=2:ry(3,4)=0
            rx(4,1)=1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=-1:ry(4,3)=-1
            rx(4,4)=-2:ry(4,4)=0
        CASE 5
            REM PURPLE T PIECE
            piecex(1)=5:piecey(1)=0
            piecex(2)=4:piecey(2)=1
            piecex(3)=5:piecey(3)=1
            piecex(4)=6:piecey(4)=1
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=1:ry(1,2)=1
            rx(1,3)=0:ry(1,3)=0
            rx(1,4)=-1:ry(1,4)=-1
            rx(2,1)=1:ry(2,1)=1
            rx(2,2)=1:ry(2,2)=-1
            rx(2,3)=0:ry(2,3)=0
            rx(2,4)=-1:ry(2,4)=1
            rx(3,1)=1:ry(3,1)=-1
            rx(3,2)=-1:ry(3,2)=-1
            rx(3,3)=0:ry(3,3)=0
            rx(3,4)=1:ry(3,4)=1
            rx(4,1)=-1:ry(4,1)=-1
            rx(4,2)=-1:ry(4,2)=1
            rx(4,3)=0:ry(4,3)=0
            rx(4,4)=1:ry(4,4)=-1
        CASE 6
            REM YELLOW O PIECE
            piecex(1)=4:piecey(1)=0
            piecex(2)=5:piecey(2)=0
            piecex(3)=4:piecey(3)=1
            piecex(4)=5:piecey(4)=1
            rx(1,1)=0:ry(1,1)=0
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=0:ry(1,3)=0
            rx(1,4)=0:ry(1,4)=0
            rx(2,1)=0:ry(2,1)=0
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=0:ry(2,3)=0
            rx(2,4)=0:ry(2,4)=0
            rx(3,1)=0:ry(3,1)=0
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=0:ry(3,3)=0
            rx(3,4)=0:ry(3,4)=0
            rx(4,1)=0:ry(4,1)=0
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=0:ry(4,3)=0
            rx(4,4)=0:ry(4,4)=0
        CASE 7
            REM WHITE I PIECE
            ' LOCATE 5,1 : PRINT "DEFINEPIECE: WHITE I PIECE"
            piecex(1)=5:piecey(1)=0
            piecex(2)=5:piecey(2)=1
            piecex(3)=5:piecey(3)=2
            piecex(4)=5:piecey(4)=3
            rx(1,1)=-1:ry(1,1)=1
            rx(1,2)=0:ry(1,2)=0
            rx(1,3)=1:ry(1,3)=-1
            rx(1,4)=2:ry(1,4)=-2
            rx(2,1)=1:ry(2,1)=-1
            rx(2,2)=0:ry(2,2)=0
            rx(2,3)=-1:ry(2,3)=1
            rx(2,4)=-2:ry(2,4)=2
            rx(3,1)=-1:ry(3,1)=1
            rx(3,2)=0:ry(3,2)=0
            rx(3,3)=1:ry(3,3)=-1
            rx(3,4)=2:ry(3,4)=-2
            rx(4,1)=1:ry(4,1)=-1
            rx(4,2)=0:ry(4,2)=0
            rx(4,3)=-1:ry(4,3)=1
            rx(4,4)=-2:Ry(4,4)=2
    END SELECT
END SUB

SUB PREVIEW
    PP=Int(Rnd(1)*7)+1

    DEFINEPIECE PP

    LINE (PREVIEW_XO, PREVIEW_YO-1)-(PREVIEW_XO + 4*SCALE_X, PREVIEW_YO + 4*SCALE_Y-1), 0, BF
    FOR P AS INTEGER = 1 TO 4
        DIM AS INTEGER X=piecex(P), Y=piecey(P)
        DIM AS INTEGER XT=(X-3)*SCALE_X+PREVIEW_XO, YT=Y*SCALE_Y+PREVIEW_YO
        LINE (XT, YT)-(XT+SCALE_X-2, YT+SCALE_Y-2), PP, BF
        LINE (XT+1,YT+1)-(XT+SCALE_X-3,YT+1),0
        LINE (XT+SCALE_X-3,YT+1)-(XT+SCALE_X-3,YT+SCALE_Y-3),0
    NEXT P
END SUB

SUB CHECKGRID
    FOR CY AS INTEGER = 0 TO 19
        DIM AS INTEGER ROW=0
        FOR X AS INTEGER = 0 TO 9
            IF grid(X,CY)<>0 THEN ROW=ROW+1
        NEXT X
        ' IF ROW=10 THEN DROPGRID
        ' IF CY=0 AND ROW>0 THEN GAMEOVERMAN
    NEXT CY
END SUB

SUB NEWPIECE
    CHECKGRID
    ROT=1
    PC=PP
    PREVIEW
    DEFINEPIECE PC
END SUB

SUB DRAWPIECE
    FOR P AS INTEGER = 1 TO 4
        DIM AS INTEGER X=piecex(P), Y=piecey(P)
        grid(X,Y)=PC
        DIM AS INTEGER XT=X*SCALE_X+GRID_XO, YT=Y*SCALE_Y+GRID_YO
        LINE (XT, YT)-(XT+SCALE_X-2, YT+SCALE_Y-2), PC, BF
        LINE (XT+1,YT+1)-(XT+SCALE_X-3,YT+1),0
        LINE (XT+SCALE_X-3,YT+1)-(XT+SCALE_X-3,YT+SCALE_Y-3),0
    NEXT P
END SUB

SUB ERASEPIECE:
    FOR P AS INTEGER = 1 TO 4
        DIM AS INTEGER X=piecex(P), Y=piecey(P)
        grid(X,Y) = 0
        DIM AS INTEGER XT=X*SCALE_X+GRID_XO, YT=Y*SCALE_Y+GRID_YO
        LINE (XT, YT)-(XT+SCALE_X-2, YT+SCALE_Y-2), 0, BF
    NEXT P
END SUB

SUB MOVEDOWN:
    ERASEPIECE
    DIM AS BOOLEAN E=FALSE
    ' bottom reached ?
    FOR P AS INTEGER = 1 TO 4
        IF piecey(P)=19 THEN E=TRUE
    NEXT P
    IF E THEN DRAWPIECE : NEWPIECE : RETURN
    ' collision ?
    FOR P AS INTEGER =1 TO 4
        IF grid(piecex(P),piecey(P)+1)<>0 THEN E=TRUE
    NEXT P
    IF E THEN DRAWPIECE : NEWPIECE : RETURN
    FOR P AS INTEGER = 1 To 4 : piecey(P)=piecey(P)+1 : NEXT P
    DRAWPIECE
    ' IF A=90 or A=122 THEN MOVEDOWN
    ' A=0
END SUB

SUB MOVERIGHT
    ERASEPIECE
    DIM AS BOOLEAN e=FALSE
    FOR p AS INTEGER = 1 TO 4
        IF piecex(p)=9 THEN e=TRUE
    NEXT p
    IF e THEN DRAWPIECE : RETURN
    FOR p AS INTEGER = 1 TO 4
        IF grid(piecex(p)+1, piecey(p))<>0 THEN e=TRUE
    NEXT p
    IF e THEN DRAWPIECE : RETURN
    FOR p AS INTEGER = 1 TO 4 : piecex(p)=piecex(p)+1 : NEXT p
    DRAWPIECE
END SUB

SUB MOVELEFT
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If piecex(P)=0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4
'     If grid(piecex(P)-1,piecey(P))<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4:piecex(P)=piecex(P)-1:Next P
'   GoSub DRAWPIECE
END SUB

SUB CPRINT (BYVAL y AS INTEGER, s AS STRING)

    DIM AS INTEGER x = 20-LEN(s)/2
    LOCATE y, x
    PRINT s

END SUB

STARTSCREEN:

FOR x AS INTEGER = 0 TO 9
    FOR y AS INTEGER = 0 TO 19
        grid(x,y) = 0
    NEXT y
NEXT x

SCREEN 2, 320, 200, 3, AS_MODE_LORES, "AQB Tetris"
WINDOW 4,,,AW_FLAG_BACKDROP OR AW_FLAG_BORDERLESS,2

PALETTE 0, 0.0, 0.0, 0.0 : REM black
PALETTE 1, 0.0, 0.0, 1.0 : REM blue
PALETTE 2, 0.0, 1.0, 0.0 : REM green
PALETTE 3, 0.0, 1.0, 1.0 : REM cyan
PALETTE 4, 1.0, 0.0, 0.0 : REM red
PALETTE 5, 1.0, 0.0, 1.0 : REM purple
PALETTE 6, 1.0, 1.0, 0.0 : REM yellow
PALETTE 7, 1.0, 1.0, 1.0 : REM white

CLS

COLOR 7

CPRINT  3, "Welcome to"
CPRINT  5, "AQB TETRIS"
CPRINT  7, "Based on COLOUR MAXIMITE TETRIS"
CPRINT  9, "By David Murray"
CPRINT 11, "AQB port by Guenter Bartsch"

LOCATE 15, 2 : PRINT "Please enter level 1-10"
LOCATE 16, 2 : PRINT "default being 2, 10 being ridiculous"
LOCATE 18, 2 : PRINT "Good Luck!"
LINE (2 ,107)-(317,147),5,B
LINE (0 ,105)-(319,149),5,B

DIM AS SINGLE t2=0

WHILE t2=0

    DIM AS STRING key = INKEY$

    IF key = "" THEN

        SLEEP

    ELSE

        SELECT CASE ASC(key)
            CASE 48    : t2= 50 : REM key 0
            CASE 49    : t2=500 : REM key 1
            CASE 50, 13: t2=450 : REM key 2 or enter
            CASE 51    : t2=400 : REM key 3
            CASE 52    : t2=350 : REM key 4
            CASE 53    : t2=300 : REM key 5
            CASE 54    : t2=250 : REM key 6
            CASE 55    : t2=200 : REM key 7
            CASE 56    : t2=150 : REM key 8
            CASE 57    : t2=100 : REM key 9
        END SELECT

        ' FIXME: IF Timer>=350 Then GoSub PREVIEW:Timer=0

    END IF
WEND

SETUPSCREEN:

COLOR 7,1
CLS

PRINT
PRINT
PRINT " LEFT/RIGHT"
PRINT " MOVE"
PRINT
PRINT
PRINT " UP"
PRINT " ROTATE"
PRINT
PRINT
PRINT " DOWN"
PRINT " DROP"
PRINT
PRINT
PRINT " ESC"
PRINT " PAUSE/EXIT"

LOCATE 4, 31 : PRINT "NEXT"
LINE (PREVIEW_XO-1, PREVIEW_YO-2)-(PREVIEW_XO+4*SCALE_X+1,PREVIEW_YO+4*SCALE_Y),7,B

LINE (GRID_XO-1, GRID_YO-1)-(GRID_XO+10*SCALE_X+1,GRID_YO+20*SCALE_Y+1),7,B
LINE (GRID_XO  , GRID_YO  )-(GRID_XO+10*SCALE_X  ,GRID_YO+20*SCALE_Y  ),0,BF

LOCATE 12, 31 : PRINT "LINES"
LINE (LINES_XO-1, LINES_YO-1)-(LINES_XO+4*SCALE_X+1, LINES_YO+2*SCALE_Y+1),7,B
LINE (LINES_XO  , LINES_YO  )-(LINES_XO+4*SCALE_X  , LINES_YO+2*SCALE_Y  ),0,BF

LOCATE 18, 31 : PRINT "SCORE"
LINE (SCORE_XO-1, SCORE_YO-1)-(SCORE_XO+4*SCALE_X+1, SCORE_YO+2*SCALE_Y+1),7,B
LINE (SCORE_XO  , SCORE_YO  )-(SCORE_XO+4*SCALE_X  , SCORE_YO+2*SCALE_Y  ),0,BF

COLOR 7,0
LOCATE 20, 33 : PRINT S
LOCATE 14, 33 : PRINT L

PREVIEW
NEWPIECE
DRAWPIECE

DIM delay AS SINGLE = t2/1000
'LOCATE 2,1 : PRINT "delay=";delay

ON TIMER CALL 1, delay, MOVEDOWN
TIMER ON 1

DIM SHARED AS BOOLEAN gameover=FALSE

WHILE NOT gameover
    SLEEP
    DIM AS STRING k = INKEY$
    IF k = "" THEN CONTINUE
    SELECT CASE ASC(k)
        CASE 27: ' Escape
            gameover = True
        CASE 29: ' cursor down
            MOVEDOWN
        CASE 30: ' cursor right
            MOVERIGHT
        CASE 31: ' cursor left
            MOVELEFT
    END SELECT
WEND

'DIM SHARED AS BOOLEAN q=FALSE
'WHILE NOT q
'    SLEEP
'WEND

' READKEYBOARD:
'   A=Asc(Inkey$)
'   If A=129 Then GoSub MOVEDOWN
'   If A=130 Then GoSub MOVELEFT
'   If A=131 Then GoSub MOVERIGHT
'   If A=122 or A=90 Then GoSub LEAVEPIECE
'   If A=128 or A=32 Then GoSub ROTATE
'   If A=27 Then GoSub ENDSUB
'   If Timer>=T2 Then GoSub MOVEDOWN:Timer=L
'   GoTo READKEYBOARD
' 
' LEAVEPIECE:
'   Gosub MOVEDOWN
'   GoSub DRAWPIECE
'   Return
' 
' ROTATE:
'   GoSub erasepiece
'   For P=1 To 4
'     piecex(P)=piecex(P)+RX(ROT,P)
'     piecey(P)=piecey(P)+RY(ROT,P)
'     Next P
'   ROT=ROT+1:If ROT=5 Then ROT=1
'   GoSub DRAWPIECE
'   Return
' 
' 
' DROPGRID:
'   GoSub CLEARSCREENgrid
'   For X=0 To 9
'     For YY= CY To 1 Step-1
'       grid(X,YY)=grid(X,YY-1)
'       Next YY,X
'   L=L+1
'   S=S+T1
'   Print @(182,106) L
'   Print @(182,155) S
'   GoSub REDRAWSCREENgrid
'   Return
' 
' CLEARSCREENgrid:
'   Line(69,14)-(159,214),0,BF
'   Return
' 
' REDRAWSCREENgrid:
'   For Y=0 To 19
'   For X=0 To 9
'   XT=X*9:YT=Y*10
'   Line(XT+70,YT+15)-(XT+77,YT+23),grid(X,Y),BF
'   Line(XT+71,YT+16)-(XT+76,YT+16),0
'   Line(XT+76,YT+16)-(XT+76,YT+22),0
'   Next X,Y
'   Return
' 
' 

' GAMEOVERMAN:
'   Print @(85,55,2) " GAME OVER"
'   Print @(85,65,2) " ---------
'   Print @(69,75,2) " WOULD YOU LIKE"
'   Print @(69,85,2) "TO PLAY ANOTHER"
'   Print @(85,95,2) " GAME Y/N?" 
'   ENDGAME:
'     E=Asc(Inkey$)
'     If E=89 or E=121 Then GoSub STARTSCREEN
'     If E=27 or E=78 or E=110 Then PRINT @(55,110,2) " THANKS FOR PLAYING ":END
'   Goto ENDGAME 
'   
' ENDSUB:
'   Print @(80,15,1) "QUITING? ARE "
'   Print @(78,25,1) "YOU SURE Y/N?" 
'   END1:
'   E=Asc(Inkey$)
'   If E=89 or E=121 Then GOSUB GAMEOVERMAN
'   If E=27 or E=78 or E=110 Then 
'     colour 0
'     Print @(80,15,1) "QUITING? ARE "
'     Print @(78,25,1) "YOU SURE Y/N?" 
'     colour 7
'     Return
'   Endif  
'   Goto END1
'   



