'
' AQB Tetris
'
' based on Colour Maximite tetris by David Murray *edited by Daedan Bamkin
' http://www.the8bitguy.com/download-davids-software/
'
' AQB port by G. Bartsch
'

OPTION EXPLICIT

'DIM AS SINGLE startTime = TIMER()
'PRINT "startTime=";startTime

' Dim GRID(10,20)
' Dim rx(4,4):Dim ry(4,4)
' Dim PIECEX(4)
' Dim PIECEY(4)

SUB PREVIEW
   LINE (182,36)-(209,77),1,BF
   DIM AS INTEGER PP=Int(Rnd(1)*7)+1
'   If PP=1 Then GoSub DEFINEPIECE1
'   If PP=2 Then GoSub DEFINEPIECE2
'   If PP=3 Then GoSub DEFINEPIECE3
'   If PP=4 Then GoSub DEFINEPIECE4
'   If PP=5 Then GoSub DEFINEPIECE5
'   If PP=6 Then GoSub DEFINEPIECE6
'   If PP=7 Then GoSub DEFINEPIECE7
'   For P=1 To 4
'     X=PIECEX(P):Y=PIECEY(P)
'     XT=X*9+77:YT=Y*10+22
'     Line(XT+70,YT+15)-(XT+77,YT+23),PP,BF
'     Line(XT+71,YT+16)-(XT+76,YT+16),0
'     Line(XT+76,YT+16)-(XT+76,YT+22),0
'     Next P
END SUB

' STARTSCREEN:

' For X=0 To 9
' For Y=0 To 19
' GRID(X,Y)=0
' Next Y,X
'
' L=0
' S=0

' COLOR 7,1

WINDOW 1, "Tetris", (0,0) - (638, 220), AW_FLAG_SIZE OR AW_FLAG_DRAG OR AW_FLAG_DEPTH OR AW_FLAG_CLOSE

CLS

LOCATE  2, 28 : PRINT "Welcome to"
LOCATE  4, 28 : PRINT "AQB TETRIS"
LOCATE  6, 12 : PRINT "Based on COLOUR MAXIMITE TETRIS"
LOCATE  8, 24 : PRINT "By David Murray"
LOCATE 10, 18 : PRINT "AQB port by Guenter Bartsch"

LOCATE 16,  4 : PRINT "Please enter level difficulty between 1-10"
LOCATE 17,  4 : PRINT "default being 2, 10 being ridiculous, Good Luck!"
LINE (10,110)-(600,145),3,B
LINE (6,108)-(604,147),3,B

DIM AS INTEGER t1=0

WHILE t1=0

    DIM AS STRING key = INKEY$

    IF key = "" THEN

        SLEEP

    ELSE

        SELECT CASE ASC(key)
            CASE 48    : t1= 50 : REM key 0
            CASE 49    : t1=500 : REM key 1
            CASE 50, 13: t1=450 : REM key 2 or enter
            CASE 51    : t1=400 : REM key 3
            CASE 52    : t1=350 : REM key 4
            CASE 53    : t1=300 : REM key 5
            CASE 54    : t1=250 : REM key 6
            CASE 55    : t1=200 : REM key 7
            CASE 56    : t1=150 : REM key 8
            CASE 57    : t1=100 : REM key 9
        END SELECT

        ' FIXME: IF Timer>=350 Then GoSub PREVIEW:Timer=0

    END IF
WEND

' Mode 4,1:Cls

CLS

SETUPSCREEN:
PRINT " AQB Tetris by David Murray, Guenter Bartsch"
PRINT
PRINT "  CURSOR                       NEXT"
PRINT "   KEYS"
PRINT "   MOVE"
PRINT
PRINT " UP/SPACE"
PRINT " to ROTATE"
PRINT " Z to DROP"
' Print @(170,190,1) "ESC to"
' Print @(170,200,1) "PAUSE/EXIT"
' Print @(183,90,1) "LINES"
' Print @(183,140,1) "SCORE"
LINE (181,35)-(210,78),2,B
LINE (68,13)-(160,215),2,B
LINE (69,14)-(159,214),1,BF
LINE (181,101)-(214,120),2,B
LINE (182,102)-(213,119),1,BF
LINE (181,151)-(214,170),2,B
LINE (182,152)-(213,169),1,BF

WHILE INKEY$=""
    SLEEP
WEND


' Colour 7,0
' Print @(182,106) S
' Print @(182,155) T1

PREVIEW

' GoSub PREVIEW
' GoSub NEWPIECE
' GoSub DRAWPIECE
' 
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
' MOVERIGHT:
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If PIECEX(P)=9 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4
'     If GRID(PIECEX(P)+1,PIECEY(P))<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4:PIECEX(P)=PIECEX(P)+1:Next P
'   GoSub DRAWPIECE
'   Return
' MOVELEFT:
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If PIECEX(P)=0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4
'     If GRID(PIECEX(P)-1,PIECEY(P))<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:Return
'   For P=1 To 4:PIECEX(P)=PIECEX(P)-1:Next P
'   GoSub DRAWPIECE
'   Return
' MOVEDOWN:
'   GoSub ERASEPIECE
'   E=0
'   For P=1 To 4:If PIECEY(P)=19 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:GoSub NEWPIECE:Return
'   For P=1 To 4
'     If GRID(PIECEX(P),PIECEY(P)+1)<>0 Then E=1
'     Next P
'   If E=1 Then GoSub DRAWPIECE:GoSub NEWPIECE:Return
'   For P=1 To 4:PIECEY(P)=PIECEY(P)+1:Next P
'   GoSub DRAWPIECE
'   If A=90 or A=122 Then GoSub MOVEDOWN
'   A=0
'   Return
' 
' ROTATE:
'   GoSub erasepiece
'   For P=1 To 4
'     PIECEX(P)=PIECEX(P)+RX(ROT,P)
'     PIECEY(P)=PIECEY(P)+RY(ROT,P)
'     Next P
'   ROT=ROT+1:If ROT=5 Then ROT=1
'   GoSub DRAWPIECE
'   Return
' 
' CHECKGRID:
'   For CY=0 To 19
'     ROW=0
'     For X=0 To 9
'       If GRID(X,CY)<>0 Then ROW=ROW+1
'       Next X
'     If ROW=10 Then GoSub DROPGRID
'     If CY=0 and ROW>0 Then Gosub GAMEOVERMAN
'   Next CY
'   Return
' 
' DROPGRID:
'   GoSub CLEARSCREENGRID
'   For X=0 To 9
'     For YY= CY To 1 Step-1
'       GRID(X,YY)=GRID(X,YY-1)
'       Next YY,X
'   L=L+1
'   S=S+T1
'   Print @(182,106) L
'   Print @(182,155) S
'   GoSub REDRAWSCREENGRID
'   Return
' 
' CLEARSCREENGRID:
'   Line(69,14)-(159,214),0,BF
'   Return
' 
' REDRAWSCREENGRID:
'   For Y=0 To 19
'   For X=0 To 9
'   XT=X*9:YT=Y*10
'   Line(XT+70,YT+15)-(XT+77,YT+23),GRID(X,Y),BF
'   Line(XT+71,YT+16)-(XT+76,YT+16),0
'   Line(XT+76,YT+16)-(XT+76,YT+22),0
'   Next X,Y
'   Return
' 
' DRAWPIECE:
'   For P=1 To 4
'   X=PIECEX(P):Y=PIECEY(P)
'   GRID(X,Y)=PC
'   XT=X*9:YT=Y*10
'   Line(XT+70,YT+15)-(XT+77,YT+23),PC,BF
'   Line(XT+71,YT+16)-(XT+76,YT+16),0
'   Line(XT+76,YT+16)-(XT+76,YT+22),0
'   Next P
'   Return
' ERASEPIECE:
'   For P=1 To 4
'   X=PIECEX(P):Y=PIECEY(P)
'   GRID(X,Y)=0
'   Line(X*9+70,Y*10+15)-(X*9+77,Y*10+23),0,BF
'   Next P
'   Return
' NEWPIECE:
'   GoSub CHECKGRID
'   ROT=1
'   PC=PP
'   GoSub PREVIEW
'   If PC=1 Then GoSub DEFINEPIECE1
'   If PC=2 Then GoSub DEFINEPIECE2
'   If PC=3 Then GoSub DEFINEPIECE3
'   If PC=4 Then GoSub DEFINEPIECE4
'   If PC=5 Then GoSub DEFINEPIECE5
'   If PC=6 Then GoSub DEFINEPIECE6
'   If PC=7 Then GoSub DEFINEPIECE7
'   Return
' 
' DEFINEPIECE1:
'   Rem BLUE J PIECE8
'   PIECEX(1)=5:PIECEY(1)=0
'   PIECEX(2)=5:PIECEY(2)=1
'   PIECEX(3)=5:PIECEY(3)=2
'   PIECEX(4)=4:PIECEY(4)=2
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=1:ry(1,3)=-1
'   rx(1,4)=2:ry(1,4)=0
'   rx(2,1)=1:ry(2,1)=1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=-1
'   rx(2,4)=0:ry(2,4)=-2
'   rx(3,1)=1:ry(3,1)=-1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=-1:ry(3,3)=1
'   rx(3,4)=-2:ry(3,4)=0
'   rx(4,1)=-1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=1:ry(4,3)=1
'   rx(4,4)=0:ry(4,4)=2
'   Return
' DEFINEPIECE2:
'   Rem GREEN S PIECE
'   PIECEX(1)=4:PIECEY(1)=0
'   PIECEX(2)=4:PIECEY(2)=1
'   PIECEX(3)=5:PIECEY(3)=1
'   PIECEX(4)=5:PIECEY(4)=2
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=-1:ry(1,3)=-1
'   rx(1,4)=0:ry(1,4)=-2
'   rx(2,1)=1:ry(2,1)=1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=1
'   rx(2,4)=-2:ry(2,4)=0
'   rx(3,1)=1:ry(3,1)=-1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=1:ry(3,3)=1
'   rx(3,4)=0:ry(3,4)=2
'   rx(4,1)=-1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=1:ry(4,3)=-1
'   rx(4,4)=2:ry(4,4)=0
'   Return
' DEFINEPIECE3:
'   Rem CYAN L PIECE
'   PIECEX(1)=4:PIECEY(1)=0
'   PIECEX(2)=4:PIECEY(2)=1
'   PIECEX(3)=4:PIECEY(3)=2
'   PIECEX(4)=5:PIECEY(4)=2
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=1:ry(1,3)=-1
'   rx(1,4)=0:ry(1,4)=-2
'   rx(2,1)=1:ry(2,1)=1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=-1
'   rx(2,4)=-2:ry(2,4)=0
'   rx(3,1)=1:ry(3,1)=-1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=-1:ry(3,3)=1
'   rx(3,4)=0:ry(3,4)=2
'   rx(4,1)=-1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=1:ry(4,3)=1
'   rx(4,4)=2:ry(4,4)=0
'   Return
' DEFINEPIECE4:
'   Rem RED Z PIECE
'   PIECEX(1)=5:PIECEY(1)=0
'   PIECEX(2)=5:PIECEY(2)=1
'   PIECEX(3)=4:PIECEY(3)=1
'   PIECEX(4)=4:PIECEY(4)=2
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=1:ry(1,3)=1
'   rx(1,4)=2:ry(1,4)=0
'   rx(2,1)=1:ry(2,1)=-1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=-1
'   rx(2,4)=-2:ry(2,4)=0
'   rx(3,1)=-1:ry(3,1)=1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=1:ry(3,3)=1
'   rx(3,4)=2:ry(3,4)=0
'   rx(4,1)=1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=-1:ry(4,3)=-1
'   rx(4,4)=-2:ry(4,4)=0
'   Return
' DEFINEPIECE5:
'   Rem PURPLE T PIECE
'   PIECEX(1)=5:PIECEY(1)=0
'   PIECEX(2)=4:PIECEY(2)=1
'   PIECEX(3)=5:PIECEY(3)=1
'   PIECEX(4)=6:PIECEY(4)=1
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=1:ry(1,2)=1
'   rx(1,3)=0:ry(1,3)=0
'   rx(1,4)=-1:ry(1,4)=-1
'   rx(2,1)=1:ry(2,1)=1
'   rx(2,2)=1:ry(2,2)=-1
'   rx(2,3)=0:ry(2,3)=0
'   rx(2,4)=-1:ry(2,4)=1
'   rx(3,1)=1:ry(3,1)=-1
'   rx(3,2)=-1:ry(3,2)=-1
'   rx(3,3)=0:ry(3,3)=0
'   rx(3,4)=1:ry(3,4)=1
'   rx(4,1)=-1:ry(4,1)=-1
'   rx(4,2)=-1:ry(4,2)=1
'   rx(4,3)=0:ry(4,3)=0
'   rx(4,4)=1:ry(4,4)=-1
'   Return
' DEFINEPIECE6:
'   Rem YELLOW O PIECE
'   PIECEX(1)=4:PIECEY(1)=0
'   PIECEX(2)=5:PIECEY(2)=0
'   PIECEX(3)=4:PIECEY(3)=1
'   PIECEX(4)=5:PIECEY(4)=1
'   rx(1,1)=0:ry(1,1)=0
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=0:ry(1,3)=0
'   rx(1,4)=0:ry(1,4)=0
'   rx(2,1)=0:ry(2,1)=0
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=0:ry(2,3)=0
'   rx(2,4)=0:ry(2,4)=0
'   rx(3,1)=0:ry(3,1)=0
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=0:ry(3,3)=0
'   rx(3,4)=0:ry(3,4)=0
'   rx(4,1)=0:ry(4,1)=0
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=0:ry(4,3)=0
'   rx(4,4)=0:ry(4,4)=0
'   Return
' DEFINEPIECE7:
'   Rem WHITE I PIECE
'   PIECEX(1)=5:PIECEY(1)=0
'   PIECEX(2)=5:PIECEY(2)=1
'   PIECEX(3)=5:PIECEY(3)=2
'   PIECEX(4)=5:PIECEY(4)=3
'   rx(1,1)=-1:ry(1,1)=1
'   rx(1,2)=0:ry(1,2)=0
'   rx(1,3)=1:ry(1,3)=-1
'   rx(1,4)=2:ry(1,4)=-2
'   rx(2,1)=1:ry(2,1)=-1
'   rx(2,2)=0:ry(2,2)=0
'   rx(2,3)=-1:ry(2,3)=1
'   rx(2,4)=-2:ry(2,4)=2
'   rx(3,1)=-1:ry(3,1)=1
'   rx(3,2)=0:ry(3,2)=0
'   rx(3,3)=1:ry(3,3)=-1
'   rx(3,4)=2:ry(3,4)=-2
'   rx(4,1)=1:ry(4,1)=-1
'   rx(4,2)=0:ry(4,2)=0
'   rx(4,3)=-1:ry(4,3)=1
'   rx(4,4)=-2:Ry(4,4)=2
'   Return
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



