OPTION EXPLICIT

CONST AS SINGLE g = 9.8 : REM gravity

DIM SHARED AS INTEGER choice=0

SUB CPrint ( BYVAL Y AS INTEGER, S AS STRING )
    
    DIM AS INTEGER X = 40 - LEN ( S ) / 2
    LOCATE Y, X
    PRINT S
    
END SUB

REM *********************************************************
REM **
REM ** GAME
REM **
REM *********************************************************

REM IMPLEMENTATION MODULE Game;
REM FROM XTerm IMPORT TERMTYPE,ESCAPE,HasColors,ResetTerm,SEQ,AskTermType,
REM                   ClrScr,PlotBox,Center,HideCursor,ShowCursor,CursorXY,
REM                   ClrEol,RandomizeShuffle,STRINGSEQ,InputCardinal;
REM FROM SYSTEM IMPORT FILL,ADR;
REM FROM Terminal IMPORT ReadChar;
REM FROM MathLib IMPORT Sin,Cos,Entier,Random;
REM FROM Convert IMPORT StrToCard;
REM FROM Strings IMPORT Length;
REM 

CONST AS INTEGER OUTCOME_NOTHING=0, OUTCOME_BUILDING=1, OUTCOME_PLAYER=2

REM TYPE OUTCOME=(NOTHING,BUILDING,PLAYER);
REM VAR t:TERMTYPE;
REM     s:ARRAY [0..79] OF CHAR;
DIM SHARED AS UBYTE   screen          (79,24)
DIM SHARED AS INTEGER buildingWidths  (9)
DIM SHARED AS INTEGER buildingHeights (9)
DIM SHARED AS INTEGER colors          (9)
REM     outcome,dummy:OUTCOME;
REM 
REM PROCEDURE ClearBrick(x,y:CARDINAL;ch:CHAR;simulation:BOOLEAN);
REM BEGIN
REM   IF (x>0) AND (x<80) AND (y>0) AND (y<25) THEN
REM     IF (screen[x][y]=1C) THEN
REM       CursorXY(x,y); WRITE(ch);
REM       IF NOT simulation THEN screen[x][y]:=0C END;
REM     END;
REM   END;
REM END ClearBrick;
REM 
REM PROCEDURE HitBuilding(x,y:CARDINAL;simulation:BOOLEAN);
REM VAR ch:CHAR;
REM     i:CARDINAL;
REM BEGIN
REM   IF simulation THEN ch:='.'; WRITE(SEQ[WHITE]); ELSE ch:=' ' END;
REM   ClearBrick(x,y-1,ch,simulation);
REM   FOR i:=x-1 TO x+1 DO ClearBrick(i,y,ch,simulation) END;
REM   ClearBrick(x,y+1,ch,simulation);
REM END HitBuilding;

FUNCTION IsInnerBuildingHit(BYVAL x AS INTEGER, BYVAL y AS INTEGER) AS BOOLEAN
    IF (x>1) AND (x<79) THEN
        RETURN (screen(x-1,y) = &H1C) AND (screen(x+1,y) = &H1C)
    ELSEIF x=1 THEN
        RETURN screen(2,y) = &H1C
    ELSE
        RETURN screen(78,y) = &H1C
    END IF
END FUNCTION : REM IsInnerBuildingHit

FUNCTION LookUpstairs(BYREF x AS INTEGER, BYREF y AS INTEGER) AS INTEGER
    WHILE (y>1) AND (screen(x,y)=&H1C) 
        y=y-1
    WEND
    IF screen(x,y) = &H2C THEN
        RETURN OUTCOME_PLAYER
    ELSE
        y = y+1
        RETURN OUTCOME_BUILDING
    END IF
END FUNCTION : REM LookUpstairs

FUNCTION TraceBullet(BYVAL j AS INTEGER, BYVAL i AS INTEGER, BYVAL y0 AS INTEGER, _
    BYVAL a AS SINGLE, BYVAL B AS SINGLE, BYVAL ch AS UBYTE,_
    BYREF finalX AS INTEGER, BYREF finalY AS INTEGER) AS INTEGER
    REM VAR x,y:REAL;
    REM     yInt:CARDINAL;
    REM     result:OUTCOME;
    REM BEGIN
    DIM AS INTEGER result = OUTCOME_NOTHING
    DIM AS SINGLE x = (j-1)/2.0
    DIM AS SINGLE y = y0 + a*x*x - B*x
    DIM AS INTEGER yInt    
    IF y >= 1000.0 THEN
        yInt = 1000
    ELSE
        yInt = y
    END IF
    IF (yInt>=1) AND (yInt<25) THEN
        IF screen(i,yInt) = &H2C THEN
            result = OUTCOME_PLAYER
        ELSEIF screen(i,yInt) = &H1C THEN
            IF IsInnerBuildingHit(i,yInt) THEN
                result = LookUpstairs(i,yInt)
            ELSE
                result = OUTCOME_BUILDING
            END IF
        ELSE
            LOCATE yInt, i : PRINT ch
        END IF
    END IF
    finalX = i
    finalY = yInt
    RETURN result
END FUNCTION : REM TraceBullet

FUNCTION Shoot(BYVAL y0 AS INTEGER, BYVAL theta AS SINGLE, BYVAL v0 AS SINGLE, _
    BYVAL xSTART AS INTEGER, BYVAL xEND AS INTEGER, BYVAL ch AS UBYTE, _
    BYREF finalX AS INTEGER, BYREF finalY AS INTEGER) AS INTEGER
    REM VAR i,j:CARDINAL;
    REM     x,y:REAL;
    REM     outcome:OUTCOME;
    REM VAR a,b:REAL;
    
    IF v0=0.0 THEN v0=1.0
    DIM AS INTEGER outcome = OUTCOME_NOTHING
    v0 = v0/5.0
    DIM AS SINGLE a = g/(2.0*v0*v0*COS(theta)*COS(theta))
    DIM AS SINGLE B = SIN(theta)/COS(theta)
    IF xSTART < xEND THEN
        DIM AS INTEGER j=1
        DIM AS INTEGER i=xSTART
        WHILE (i<=xEND) AND (outcome=OUTCOME_NOTHING)
            outcome = TraceBullet(j,i,y0,a,B,ch,finalX,finalY)
            j = j+1 : i = i + 1
        WEND
    ELSE
        DIM AS INTEGER j = 1
        DIM AS INTEGER i = xSTART
        WHILE (i>=xEND) AND (outcome=OUTCOME_NOTHING)
            outcome = TraceBullet(j,i,y0,a,B,ch,finalX,finalY)
            j=j+1 : i=i-1            
        WEND
    END IF
    RETURN outcome
END FUNCTION : REM Shoot

FUNCTION Deg2Rad (BYVAL deg AS INTEGER) AS SINGLE
    REM CONST pi=3.14159265;
    REM BEGIN
    RETURN (pi/180.0) * deg
END FUNCTION : REM Deg2Rad

REM PROCEDURE ArmUpLeft(x,y:INTEGER);
REM BEGIN
REM   CursorXY(x-1,y-2); WRITE('\');
REM   CursorXY(x-1,y-1); WRITE(' ');
REM END ArmUpLeft;
REM 
REM PROCEDURE ArmDownLeft(x,y:INTEGER);
REM BEGIN
REM   CursorXY(x-1,y-2); WRITE(' ');
REM   CursorXY(x-1,y-1); WRITE('-');
REM END ArmDownLeft;
REM 
REM PROCEDURE ArmUpRight(x,y:INTEGER);
REM BEGIN
REM   CursorXY(x+1,y-2); WRITE('/');
REM   CursorXY(x+1,y-1); WRITE(' ');
REM END ArmUpRight;
REM 
REM PROCEDURE ArmDownRight(x,y:INTEGER);
REM BEGIN
REM   CursorXY(x+1,y-2); WRITE(' ');
REM   CursorXY(x+1,y-1); WRITE('-');
REM END ArmDownRight;
REM 
SUB DrawPlayer(BYVAL x AS INTEGER, BYVAL y AS INTEGER)
    REM VAR i,j:INTEGER;
    COLOR 2    
    REM   WRITE(SEQ[WHITE]);
    LOCATE y-2, x-1 : PRINT " o "
    LOCATE y-1, x-1 : PRINT "-|-"
    LOCATE   y, x-1 : PRINT "/ \"
    REM   screen[x][y-2]:=2C;
    REM   FOR i:=x-1 TO x+1 DO
    REM     FOR j:=y-1 TO y DO
    REM       screen[i][j]:=2C
    REM     END
    REM   END
END SUB : REM DrawPlayer

REM PROCEDURE KillPlayer(x,y:INTEGER);
REM VAR i,j:INTEGER;
REM BEGIN
REM   WRITE(SEQ[WHITE],SEQ[BLINK]);
REM   CursorXY(x-2,y-2); WRITE('-\|/-');
REM   CursorXY(x-2,y-1); WRITE('-*X*-');
REM   CursorXY(x-2,y-0); WRITE('-/|\-');
REM   WRITE(SEQ[NOBLINK]);
REM END KillPlayer;
REM 
REM PROCEDURE WinnerPlayer(x,y:INTEGER);
REM VAR i,j:INTEGER;
REM BEGIN
REM   WRITE(SEQ[WHITE]);
REM   CursorXY(x-1,y-2); WRITE('\o/');
REM   CursorXY(x-1,y-1); WRITE(' | ');
REM   CursorXY(x-1,y-0); WRITE('/ \');
REM END WinnerPlayer;
REM 
SUB DrawBuilding(BYREF c AS INTEGER, BYVAL x AS INTEGER, BYVAL h AS INTEGER, w AS INTEGER)
    
    REM PRINT "DrawBuilding: c=";c;", x=";x;",h=";h;",w=";w    
    
    REM VAR i,j,count:CARDINAL;
    REM     line:ARRAY [0..15] OF CHAR;
    REM BEGIN
    
    COLOR c
    
    REM   WRITE(color);
    REM   FILL(ADR(line),15,ORD(block));
    REM   line[w]:=0C;
    IF (h=0) OR (w=0) THEN RETURN
    DIM AS INTEGER count = 0
    FOR i AS INTEGER = 24-h TO 23 
        count = count + 1
        REM     CursorXY(x,i);
        REM IF (i=24) OR (i=25-h) OR (count MOD 2=1) THEN
        
        REM PRINT "   LINE at (";x*4;",";i*4;")"            
        LINE (x*8, i*8) - ((x+w)*8-1, i*8+7), c, BF             
        REM       WRITE(SEQ[REVERSE],line);
        REM END IF
        FOR j AS INTEGER = 1 TO w
            REM       screen[x+j-1,i]:=1C;
            IF (i<23) AND (count MOD 2=0) THEN
                IF (j=w) OR (j MOD 2=1) THEN
                    LINE ((x+j)*8, i*8) - ((x+j)*8+7, i*8+7), 0, BF                    
                    REM           WRITE(SEQ[REVERSE],block);
                    REM ELSE
                    REM           WRITE(SEQ[PLAIN],' ');
                END IF
            END IF
        NEXT j
    NEXT i
    REM   WRITE(SEQ[PLAIN],SEQ[NODARK],SEQ[WHITE]);
END SUB : REM DrawBuilding

SUB InitBuildingDimensions
    REM VAR i,j,t:CARDINAL;
    REM     ts:STRINGSEQ;
    REM BEGIN
    buildingWidths[0] = 7
    buildingWidths[1] = 9
    buildingWidths[2] = 5
    buildingWidths[3] = 7
    buildingWidths[4] = 7
    buildingWidths[5] = 5
    buildingWidths[6] = 7
    buildingWidths[7] = 9
    buildingWidths[8] = 7
    buildingWidths[9] = 7
    colors[0] = 1
    colors[1] = 2    
    colors[2] = 3
    colors[3] = 4
    colors[4] = 5
    colors[5] = 6
    colors[6] = 7
    colors[7] = 1
    colors[8] = 2
    colors[9] = 3
    FOR i AS INTEGER  = 0 TO 9
        DIM AS INTEGER j = RND(1)*9
        DIM AS INTEGER t = buildingWidths(i)
        buildingWidths(i) = buildingWidths(j)
        buildingWidths(j) = t
        REM     j:=Entier(Random()*10.0);
        REM     ts:=colors[i];
        REM     colors[i]:=colors[j];
        REM     colors[j]:=ts;
        buildingHeights(i) = RND(1)*15+5
        REM PRINT "InitBuildingDimensions: i=";i;", j=";j;", h=";buildingHeights(i)        
    NEXT i
    REM   FILL(ADR(screen),1896,0);
    
END SUB : REM InitBuildingDimensions

SUB DrawBuildings
    DIM AS INTEGER x = 0
    FOR i AS INTEGER = 0 TO 9
        DrawBuilding colors(i), x, buildingHeights(i), buildingWidths(i)
        x = x + buildingWidths(i)+1
    NEXT i
END SUB : REM DrawBuildings

SUB SetInitialPlayersPosition(BYREF x1 AS INTEGER, BYREF y1 AS INTEGER, _
    BYREF x2 AS INTEGER, BYREF y2 AS INTEGER)
    
    REM                                     VAR widths,heights:ARRAY OF CARDINAL);
    REM VAR b,i,space:CARDINAL;
    REM BEGIN
    DIM AS INTEGER B = RND(1)*3.0
    DIM AS INTEGER space = 0
    FOR i AS INTEGER = 0 TO B
        space = space + buildingWidths(i)+1
    NEXT i
    x1 = space - 1 - (buildingWidths(B) / 2)
    y1 = 24-buildingHeights(B)
    
    REM PRINT "P1:",x1,y1
    REM SLEEP FOR 5    
    
    B = RND(1)*3.0
    space = 0
    FOR i AS INTEGER = 9 TO (9-B) STEP -1
        space  = space+buildingWidths(i)+1
    NEXT i
    x2 = 80-(space-1-(buildingWidths(9-B) / 2))
    y2 = 24-BuildingHeights(9-B)
END SUB : REM SetInitialPlayersPosition

REM PROCEDURE PrintOutcome(o:OUTCOME);
REM BEGIN
REM   CASE o OF
REM     NOTHING: WRITE('NOTHING') |
REM     BUILDING: WRITE('BUILDING') |
REM     PLAYER: WRITE('PLAYER')
REM   END;
REM END PrintOutcome;
REM 
REM PROCEDURE ShowScore(s1,s2:CARDINAL);
REM BEGIN
REM   CursorXY(35,24); WRITE(SEQ[WHITE],' ',s1:0,'>Score<',s2:0,' ');
REM   CursorXY(35,23);
REM END ShowScore;
REM 
REM PROCEDURE Celebrate(VAR name:ARRAY OF CHAR);
REM VAR ch:CHAR;
REM BEGIN
REM   CursorXY(20,1);
REM   WRITE(SEQ[WHITE],SEQ[REVERSE],' ',name,' ',SEQ[PLAIN],
REM     ' won this battle! Press any key ');
REM   ReadChar(ch);
REM END Celebrate;

SUB StartGame
    
    DIM AS INTEGER angle1, angle2, speed1, speed2
    REM     ch:CHAR;
    REM     posName2:CARDINAL;
    REM     x1,y1,x2,y2,finalX,finalY:CARDINAL;
    DIM AS INTEGER winner = 0
    DIM AS INTEGER score1=0, score2=0
    
    RANDOMIZE TIMER    
    
    REM   posName2:=80-Length(playerName2);
    
    REM DO : REM round loop    
    
    CLS        
    REM     WRITE(SEQ[WHITE]);
    REM     finalX:=0; finalY:=0;
    
    InitBuildingDimensions
    DrawBuildings
    
    DIM AS INTEGER x1, y1, x2, y2    
    
    SetInitialPlayersPosition x1, y1, x2, y2
    DrawPlayer x1, y1
    DrawPlayer x2, y2
    
    REM FIXME ShowScore(score1,score2);
    
    REM     LOOP (* Single battle *)
    REM       IF winner#2 THEN
    REM         ShowCursor;
    LOCATE 1,1 : PRINT "PLAYER 1"
    LOCATE 2,1 : INPUT "Angle: ", angle1 
    REM angle1 = 30 : LOCATE 2,1 : PRINT "Angle: "; angle1    
    REM FIXME
    speed1 = 10 : LOCATE 3,1 : PRINT "Speed: "; speed1
    
    
    REM         CursorXY(1,1); WRITE(SEQ[DARK],playerName1,SEQ[NODARK]);
    REM         CursorXY(1,2); WRITE("Angle:");
    REM         IF NOT InputCardinal(8,2,angle1,3) THEN RETURN END;
    REM         CursorXY(1,3); WRITE("Speed:");
    REM         IF NOT InputCardinal(8,3,speed1,4) THEN RETURN END;
    REM         HideCursor;
    REM         CursorXY(1,1); ClrEol;
    REM         CursorXY(1,2); ClrEol;
    REM         CursorXY(1,3); ClrEol;
    REM         IF (finalX#0) AND (finalY#0) THEN
    REM           ArmDownRight(x2,y2);
    REM           dummy:=Shoot(y2-2,Deg2Rad(angle2),
    REM                        FLOAT(speed2),x2-1,1,' ',finalX,finalY);
    REM           HitBuilding(finalX,finalY,FALSE);
    REM         END;
    REM         ArmUpLeft(x1,y1);
    DIM AS INTEGER finalX=0, finalY=0    
    DIM AS INTEGER outcome = Shoot(y1-2,Deg2Rad(angle1), speed1, x1+1, 79, _
    ASC("*"), finalX, finalY)
    REM         IF outcome=PLAYER THEN
    REM           winner:=1; finalX:=0; finalY:=0;
    REM           INC(score1);
    REM           KillPlayer(x2,y2);
    REM           WinnerPlayer(x1,y1);
    REM           ShowScore(score1,score2);
    REM           Celebrate(playerName1);
    REM           EXIT;
    REM         END;
    REM         IF outcome=BUILDING THEN
    REM           HitBuilding(finalX,finalY,TRUE);
    REM         END;
    REM         IF finalY>20 THEN ShowScore(score1,score2) END;
    REM       END;
    REM 
    REM       ShowCursor;
    REM       CursorXY(posName2,1); WRITE(SEQ[DARK],playerName2,SEQ[NODARK]);
    REM       CursorXY(69,2); WRITE("Angle:    ");
    REM       IF NOT InputCardinal(76,2,angle2,3) THEN RETURN END;
    REM       CursorXY(69,3); WRITE("Speed:     ");
    REM       IF NOT InputCardinal(76,3,speed2,4) THEN RETURN END;
    REM       HideCursor;
    REM       CursorXY(1,1); ClrEol;
    REM       CursorXY(1,2); ClrEol;
    REM       CursorXY(1,3); ClrEol;
    REM       IF (finalX#0) AND (finalY#0) THEN
    REM         ArmDownLeft(x1,y1);
    REM         dummy:=Shoot(y1-2,Deg2Rad(angle1),
    REM                      FLOAT(speed1),x1+1,79,' ',finalX,finalY);
    REM         HitBuilding(finalX,finalY,FALSE);
    REM       END;
    REM       ArmUpRight(x2,y2);
    REM       outcome:=Shoot(y2-2,Deg2Rad(angle2),
    REM                      FLOAT(speed2),x2-1,1,'*',finalX,finalY);
    REM       IF outcome=PLAYER THEN
    REM         winner:=2; finalX:=0; finalY:=0;
    REM         INC(score2);
    REM         KillPlayer(x1,y1);
    REM         WinnerPlayer(x2,y2);
    REM         ShowScore(score1,score2);
    REM         Celebrate(playerName2);
    REM         EXIT;
    REM       END;
    REM       IF outcome=BUILDING THEN
    REM         HitBuilding(finalX,finalY,TRUE);
    REM       END;
    REM       IF finalY>20 THEN ShowScore(score1,score2) END;
    REM 
    REM       winner:=0;
    REM     END;
    
    REM LOOP UNTIL score1+score2>=totalScore
    
    REM: FIXME remove debug code
    PRINT "*** PRESS ANY KEY ***"
    WHILE INKEY$=""
        SLEEP
    WEND
    
    REM   ClrScr;
    REM   WRITE(SEQ[RED]);
    REM   PlotBox(2,2,79,23,TRUE,TRUE);
    REM   WRITE(SEQ[YELLOW]); Center(7,'GAME OVER!');
    REM   WRITE(SEQ[CYAN]); Center(9,'Score:');
    REM   CursorXY(30,11); WRITE(SEQ[WHITE],playerName1);
    REM   CursorXY(50,11); WRITE(SEQ[GREEN],score1:0);
    REM   CursorXY(30,12); WRITE(SEQ[WHITE],playerName2);
    REM   CursorXY(50,12); WRITE(SEQ[GREEN],score2:0);
    REM   WRITE(SEQ[WHITE]); Center(15, 'Press any key...');
    REM   ReadChar(ch);
    REM   ClrScr;
    
END SUB 

REM 
REM BEGIN
REM   block:=' ';
REM   g:=9.8;
REM   totalScore:=3;
REM   playerName1:='Player 1'; playerName2:='Player 2';
REM END Game.
REM 

REM *********************************************************
REM **
REM ** MAIN
REM **
REM *********************************************************



REM PROCEDURE Clear(x,y:CARDINAL);
REM CONST s
REM   ='                                                                       ';
REM VAR i:CARDINAL;
REM BEGIN
REM  FOR i:=x TO y DO
REM    CursorXY(5,i);
REM    WRITE(s);
REM  END
REM END Clear;
REM 
REM PROCEDURE IntroScreen();
REM CONST x=8;
REM CONST y=5;
REM BEGIN
REM   IF choose=1 THEN
REM     ClrScr;
REM     WRITE(SEQ[RED]);
REM     PlotBox(4,2,77,23,FALSE,TRUE);
REM     PlotBox(3,2,78,23,TRUE,TRUE)
REM   ELSE
REM     Clear(3,22)
REM   END;
REM 
REM   WRITE(SEQ[YELLOW]);
REM 
REM   CursorXY(x+46,y+0);
REM   WRITE(SEQ[REVERSE],'     ',SEQ[PLAIN],'    ',SEQ[REVERSE],'  ',SEQ[PLAIN],'    ',
REM   SEQ[REVERSE],'    ');
REM 
REM   CursorXY(x+46,y+1);
REM   WRITE(SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',
REM   SEQ[REVERSE],'    ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',
REM   SEQ[REVERSE],'  ');
REM 
REM   CursorXY(x+46,y+2);
REM   WRITE(SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',
REM   SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',SEQ[REVERSE],'  ');
REM 
REM   CursorXY(x,y+3);
REM   WRITELN(SEQ[PLAIN],' ',SEQ[REVERSE],'    ',SEQ[PLAIN],'   ',SEQ[REVERSE],'    ',
REM   SEQ[PLAIN],'  ',SEQ[REVERSE],'     ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',
REM   SEQ[PLAIN],' ',SEQ[REVERSE],'  ',SEQ[PLAIN],'    ',SEQ[REVERSE],'  ',SEQ[PLAIN],
REM   '     ',SEQ[REVERSE],'    ',SEQ[PLAIN],'     ',SEQ[REVERSE],'     ',SEQ[PLAIN],
REM   '  ',SEQ[REVERSE],'      ',SEQ[PLAIN],'  ',SEQ[REVERSE],'    ');
REM 
REM   CursorXY(x,y+4);
REM   WRITELN(SEQ[REVERSE],'  ',SEQ[PLAIN],'     ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',
REM   SEQ[REVERSE],'  ',SEQ[PLAIN],' ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],
REM   '  ',SEQ[PLAIN],' ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',SEQ[REVERSE],'  ',SEQ[PLAIN],
REM   '    ',SEQ[REVERSE],'  ',SEQ[PLAIN],'    ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',
REM   SEQ[REVERSE],'  ',SEQ[PLAIN],'    ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',
REM   SEQ[REVERSE],'  ',SEQ[PLAIN],' ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],
REM   '  ',SEQ[PLAIN],'     ',SEQ[REVERSE],'  ');
REM 
REM   CursorXY(x,y+5);
REM   WRITE(SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',
REM   SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',
REM   SEQ[REVERSE],'     ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',
REM   SEQ[REVERSE],'  ',SEQ[PLAIN],'    '
REM   ,SEQ[REVERSE],'  ',SEQ[PLAIN],'    ',
REM   SEQ[REVERSE],'      ',SEQ[PLAIN],' ',SEQ[PLAIN] ,'  ',SEQ[PLAIN],' ',
REM   SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ', SEQ[REVERSE],
REM   '  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',SEQ[REVERSE],'  ',SEQ[PLAIN],
REM   '  ',SEQ[REVERSE],'  ');
REM 
REM   CursorXY(x,y+6);
REM   WRITE(SEQ[PLAIN],' ',SEQ[REVERSE],'    ',SEQ[PLAIN],'   ',SEQ[REVERSE],'    ',
REM   SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',
REM   SEQ[REVERSE],'  ',SEQ[PLAIN],' ',SEQ[REVERSE],'     ',SEQ[PLAIN],' ',SEQ[REVERSE],
REM   '     ',SEQ[PLAIN],' ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',
REM   SEQ[PLAIN],' ',SEQ[REVERSE],'  ',SEQ[PLAIN],' ',SEQ[REVERSE],'     ',SEQ[PLAIN],
REM   '  ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',SEQ[REVERSE],'  ',SEQ[PLAIN],'  ',
REM   SEQ[REVERSE],'    ',SEQ[PLAIN]);
REM 
REM   WRITE(SEQ[CYAN]);
REM   CursorXY(x+1,y+0); WRITE('CP/M-80 & Turbo Modula-2 version (C) 2015');
REM   CursorXY(x+1,y+1); WRITE('Microsoft DOS 5.0 QBASIC version (C) 1991');
REM   Center(13,'Written by Francesco Sblendorio');
REM END IntroScreen;
REM 

SUB SelectionScreen
    
    CLS
    COLOR 1    
    
    CPrint 10, "Make your choice"
    
    CPrint 12, "1. Start Game                 "
    CPrint 13, "2. Set Preferences            "
    CPrint 14, "3. About banana.bas           "
    CPrint 16, "4. Exit                       "
    
    choice = 0    
    
    WHILE choice=0
        DIM AS STRING ch=""        
        WHILE ch=""
            SLEEP
            ch = INKEY$            
        WEND
        
        SELECT CASE ASC(ch)
        CASE 49:
            choice = 1
        CASE 50:
            choice = 2
        CASE 51:
            choice = 3
        CASE 52:
            choice = 4
            
        END SELECT
    WEND
END SUB

REM 
REM PROCEDURE Preferences();
REM VAR ch:CHAR;
REM CONST s='               ';
REM BEGIN
REM   Clear(15,20);
REM   HideCursor();
REM   WRITE(SEQ[WHITE]);
REM   CursorXY(26,15);
REM   WRITE('1. Player 1 name: ',SEQ[DARK],playerName1,SEQ[NODARK]);
REM   CursorXY(26,16);
REM   WRITE('2. Player 2 name: ',SEQ[DARK],playerName2,SEQ[NODARK]);
REM   CursorXY(26,17);
REM   WRITE('3. Total  points: ',SEQ[DARK],totalScore:0,SEQ[NODARK]);
REM   CursorXY(26,18);
REM   WRITE('4. Gravity m/s^2: ',SEQ[DARK],g:0:1,SEQ[NODARK]);
REM   CursorXY(26,19);
REM   WRITE('5. Change block char. Current: "',SEQ[REVERSE],block,SEQ[PLAIN],'"');
REM   CursorXY(26,20);
REM   WRITE('6. Back to main menu');
REM   LOOP
REM     ReadChar(ch);
REM     IF (ch='6') OR (ch=33C) OR (ch=3C) THEN
REM       choose:=-1; RETURN;
REM     ELSIF (ch='1') THEN
REM       CursorXY(44,15);
REM       WRITE(SEQ[DARK],SEQ[REVERSE],s);
REM       ShowCursor; CursorXY(44,15);
REM       ReadLine(playerName1); HideCursor;
REM       IF playerName1='' THEN playerName1:='Player 1' END;
REM       CursorXY(44,15);
REM       WRITE(SEQ[PLAIN],SEQ[DARK],s);
REM       CursorXY(44,15);
REM       WRITE(playerName1,SEQ[NODARK]);
REM     ELSIF (ch='2') THEN
REM       CursorXY(44,16);
REM       WRITE(SEQ[DARK],SEQ[REVERSE],s);
REM       ShowCursor; CursorXY(44,16);
REM       ReadLine(playerName2); HideCursor;
REM       IF playerName2='' THEN playerName2:='Player 2' END;
REM       CursorXY(44,16);
REM       WRITE(SEQ[PLAIN],SEQ[DARK],s);
REM       CursorXY(44,16);
REM       WRITE(playerName2,SEQ[NODARK]);
REM     ELSIF (ch='3') THEN
REM       CursorXY(44,17);
REM       WRITE(SEQ[DARK],SEQ[REVERSE],s);
REM       ShowCursor; CursorXY(44,17);
REM       READ(totalScore); HideCursor;
REM       IF totalScore=0 THEN totalScore:=3 END;
REM       CursorXY(44,17);
REM       WRITE(SEQ[PLAIN],SEQ[DARK],s);
REM       CursorXY(44,17);
REM       WRITE(totalScore:0,SEQ[NODARK]);
REM     ELSIF (ch='4') THEN
REM       CursorXY(44,18);
REM       WRITE(SEQ[DARK],SEQ[REVERSE],s);
REM       ShowCursor; CursorXY(44,18);
REM       READ(g); HideCursor;
REM       IF g=0.0 THEN g:=9.8 END;
REM       CursorXY(44,18);
REM       WRITE(SEQ[PLAIN],SEQ[DARK],s);
REM       CursorXY(44,18);
REM       WRITE(g:0:1,SEQ[NODARK]);
REM     ELSIF (ch='5') THEN
REM       IF block=40C THEN
REM         block:=43C;
REM       ELSIF block=43C THEN
REM         block:=333C;
REM       ELSIF block=333C THEN
REM         block:=377C;
REM       ELSIF block=377C THEN
REM         block:=40C;
REM       END;
REM       CursorXY(57,19);
REM       WRITE('"',SEQ[REVERSE],block,SEQ[PLAIN],'"');
REM     END;
REM   END;
REM   choose:=-1;
REM   ReadChar(ch);
REM END Preferences;
REM 
REM PROCEDURE InfoScreen();
REM VAR ch:CHAR;
REM BEGIN
REM   Clear(3,22);
REM   WRITE(SEQ[YELLOW],SEQ[REVERSE]);
REM   Center(4,'                           ');
REM   Center(5,'   G O R I L L A . B A S   ');
REM   Center(6,'                           ');
REM   CursorXY(40,8);
REM   WRITE(SEQ[CYAN],SEQ[PLAIN]);
REM   Center(8,'written by');
REM   Center(9,'Francesco Sblendorio');
REM   WRITE(SEQ[LIGHTBLUE],SEQ[UNDERLINE]);
REM   Center(10,'http://www.sblendorio.eu');
REM   WRITE(SEQ[NOUNDERLINE],SEQ[CYAN]);
REM   Center(12,
REM    'CP/M & Modula-2 version (C) 2015. Based on Microsoft QBASIC game');
REM   WRITE(SEQ[WHITE]);
REM   CursorXY(7,15); WRITE(
REM   'The game consists in two gorillas throwing explosive bananas at each');
REM   CursorXY(7,17); WRITE(
REM   'other above a city skyline. The players can adjust the angle and ve-');
REM   CursorXY(7,19); WRITE(
REM   'locity of each throw.');
REM   CursorXY(40,21); WRITE('(Wikipedia - ',SEQ[LIGHTBLUE],SEQ[UNDERLINE],
REM   'https://goo.gl/TtzH9S');
REM   WRITE(SEQ[WHITE],SEQ[NOUNDERLINE],SEQ[PLAIN],')');
REM   ReadChar(ch);
REM END InfoScreen;
REM 
REM PROCEDURE DoExit();
REM VAR ch:CHAR;
REM BEGIN
REM   Clear(15,20);
REM   WRITE(SEQ[WHITE]);
REM   Center(17,'Are you sure you want to exit?');
REM   Center(19,'(Y/N)');
REM   choose:=0;
REM   REPEAT
REM     ReadChar(ch);
REM     IF (ch='y') OR (ch='Y') OR (ch='1') THEN
REM       choose:=4
REM     ELSIF (ch='n') OR (ch='N') OR (ch='0') OR (ch=33C) OR (ch=3C) THEN
REM       choose:=-1
REM     END
REM   UNTIL choose#0;
REM END DoExit;
REM 
REM PROCEDURE ExitScreen();
REM BEGIN
REM   WRITE(SEQ[WHITE]);
REM   ResetTerm();
REM   ClrScr;
REM   ShowCursor();
REM END ExitScreen;
REM 
'BEGIN
'  AskTermType();
'  RandomizeShuffle();

REM
REM Main
REM

REM SCREEN 1, 640, 200, 8, AS_MODE_HIRES

WINDOW 1, "Banana", (0,0)-(640, 200), AW_FLAG_ACTIVATE OR AW_FLAG_DRAGBAR OR _
AW_FLAG_GIMMEZEROZERO OR AW_FLAG_DEPTHGADGET OR AW_FLAG_CLOSEGADGET

DO
    REM IF choose=-1 THEN Clear(15,21) ELSE IntroScreen END;
    SelectionScreen
    SELECT CASE choice
    CASE 1
        StartGame
    CASE 2 
        REM Preferences
    CASE 3
        REM InfoScreen 
    CASE 4
        REM DoExit
    END SELECT
LOOP UNTIL choice=4
REM ExitScreen;

