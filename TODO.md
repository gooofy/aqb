# AQB TODO
Guenter Bartsch <guenter@zamia.org>

## BUGS


## Documentation, Tutorials

- DONE online help
- DONE translate forum posts, add useful information to README
- GadToolsSupport
- adapt all tutorials to 0.9.0
- OS* modules

## Language Concepts

- subprograms

    * check forward decl formal types
    * varargs

- OOP
    * interfaces
    * virtual functions

- enums

- assignment operators (+=, ...)

- C like strings (tbd)

## Compiler, Code Generation

- scanner: support multiple instances (IDE uses separate scanner instance)

- code generator
    * proc calls: only save registers that carry live values
    * use movem to save/restore registers
    * only save/restore registers on proc entry/exit that are actually used inside
    * peephole optimizer
    * cache (var) pointer temps
    * escape analysis: keep local variables in registers only (m68kframe inReg)
    * optimizing assembler (32 bit condition jumps, clr., ...)
    * frame/var size >32KByte support
    * register arguments
    * SSA (static single assignment) intermediate form
    * backend abstraction layer, enable alternative backends (RISC-V, ARM, x86, LLVM, ...)
    * if / condition constant propagation (test w/ while)
    * fix spilling
    * translate.c: add #0, tmp ; mul \*1 ; div / 1 ; ...
    * optionally: initialize all variables to 0?
    * T\_Cast that results in NOP or simple MOVE: introduce T\_TypeView node

- create Icons for Binaries

## Runtime

- do stackswap only on demand

- switch \_brt, \_aqb to commonrt

- switch IDE to commonrt

- DONE startup code: workbench startup

- startup code: command line args

- Exec error handling / traps
    * division by zero

- Memory management / garbage collection ? / DEALLOCATE

- string handling

- collections

- DONE CTRL+C to stop the program at any time

- DONE source level debugging

- DONE tetris game freezes after some 3 minutes (ressource leak? )

- MOD / protracker playback (maybe using https://aminet.net/package/mus/play/ptplayer)

- open RTG custom screens

- GadToolsSupport
	- DONE BUTTON
	- DONE CHECKBOX
	-      CYCLE
	- DONE INTEGER
	-      LISTVIEW
	-      MX
	-      NUMBER
	-      PALETTE
	- DONE SCROLLER
	- DONE SLIDER
	-      STRING
	-      TEXT

    -      Menus
    -      Keyboard shortcuts

- gtlayout.library

- asl.library

## IDE

- DONE block operations
    * DONE  mark
    * DONE cut/copy/paste

- linux TUI file dialog

- find/replace dialog
    * replace
    * port TUI to amiga gadtools

- horizontal scrolling

- mouse cursor positioning

- mark block in the IDE #28 https://github.com/gooofy/aqb/issues/28

- DONE workbench arguments (double click source code -> open file in IDE)

- DONE auto-indent
- DONE save
- DONE compile/run
- \_\_\_\_ memory management
- \_\_\_\_ help system
- DONE amiga menus
- DONE DEL
- DONE search
- \_\_\_\_ replace
- \_\_\_\_ block operations
- DONE goto line
- DONE show compiler error messages
- \_\_\_\_ terminal: use uint16\_t where possible

## Core Commands

- DONE ABS()
- DONE AND
- \_\_\_\_ ASC()
- DONE ATN()
- \_\_\_\_ BREAK ON
- \_\_\_\_ BREAK OFF
- \_\_\_\_ BREAK STOP
- DONE CALL
- \_\_\_\_ CBDL()
- \_\_\_\_ CHDIR
- DONE CHR$()
- DONE CINT()
- DONE CLEAR
- DONE CLNG()
- DONE CLOSE
- DONE CLS
- \_\_\_\_ COMMON
- DONE COS()
- \_\_\_\_ CSNG()
- \_\_\_\_ CVD()
- \_\_\_\_ CVI()
- \_\_\_\_ CVL()
- \_\_\_\_ CVS()
- DONE DATA
- DONE DATE$()
- DONE DECLARE FUNCTION
- DONE DECLARE SUB
- \_\_\_\_ DEF FN
- \_\_\_\_ DEFDBL
- DONE DEFINT
- DONE DEFLNG
- DONE DEFSNG
- DONE DEFSTR
- DONE DIM
- DONE END
- DONE EOF()
- DONE EQV
- DONE ERASE
- \_\_\_\_ ERL
- DONE ERR
- DONE ERROR
- DONE EXP()
- \_\_\_\_ FIELD
- \_\_\_\_ FILES
- DONE FIX()
- DONE FOR...NEXT
- DONE FRE()
- \_\_\_\_ GET#
- DONE GOSUB
- DONE GOTO
- \_\_\_\_ HEX$()
- DONE IF
- \_\_\_\_ IMP
- DONE INPUT
- \_\_\_\_ INPUT$()
- DONE INPUT #
- DONE INSTR()
- DONE INT()
- \_\_\_\_ KILL
- DONE LBOUND()
- DONE LEFT$()
- DONE LEN()
- DONE LET
- \_\_\_\_ LIBRARY
- \_\_\_\_ LIBRARY CLOSE
- DONE LINE INPUT
- DONE LINE INPUT#
- \_\_\_\_ LLIST
- \_\_\_\_ LOC()
- \_\_\_\_ LOF()
- DONE LOCATE
- DONE LOG()
- \_\_\_\_ LPOS()
- \_\_\_\_ LSET
- DONE MID$()
- \_\_\_\_ MKI$()
- \_\_\_\_ MKL$()
- \_\_\_\_ MKS$()
- \_\_\_\_ MKD$()
- DONE MOD
- \_\_\_\_ NAME
- DONE NEXT
- DONE NOT
- \_\_\_\_ OCT$()
- \_\_\_\_ ON BREAK
- DONE ON ERROR
- \_\_\_\_ ON GOSUB
- \_\_\_\_ ON GOTO
- DONE OPEN
- \_\_\_\_ OPTION BASE
- \_\_\_\_ OR
- DONE PEEK()
- DONE PEEKL()
- DONE PEEKW()
- DONE POKE
- DONE POKEL
- DONE POKEW
- \_\_\_\_ PUT
- DONE PRINT
- DONE PRINT \#
- \_\_\_\_ PRINT USING
- DONE RANDOMIZE
- DONE READ
- DONE REM
- DONE RESTORE
- \_\_\_\_ RESUME
- DONE RETURN
- DONE RIGHT$()
- DONE RND()
- \_\_\_\_ RSET
- \_\_\_\_ SADD()
- \_\_\_\_ SAVE
- \_\_\_\_ SGN()
- \_\_\_\_ SHARED
- DONE SIN()
- \_\_\_\_ SPACE$()
- \_\_\_\_ SPC()
- DONE SQR()
- DONE STATIC
- DONE STR$()
- \_\_\_\_ STRING$()
- \_\_\_\_ SUB
- \_\_\_\_ SWAP
- DONE SYSTEM
- \_\_\_\_ TAB()
- DONE TAN()
- \_\_\_\_ TIME$()
- DONE UBOUND()
- DONE UCASE$()
- DONE VAL()
- DONE VARPTR()
- DONE WEND
- DONE WHILE
- \_\_\_\_ WIDTH
- DONE WRITE
- DONE WRITE#

## AmigaBASIC Specific Commands

- DONE AREA
- DONE AREAFILL
- DONE AREA OUTLINE
- \_\_\_\_ BEEP
- DONE CIRCLE
- \_\_\_\_ COLLISION ON
- \_\_\_\_ COLLISION OFF
- \_\_\_\_ COLLISION STOP
- \_\_\_\_ COLLISION()
- DONE COLOR
- DONE CSRLIN()
- DONE GET
- DONE INKEY$
- DONE LINE
- \_\_\_\_ LPRINT
- \_\_\_\_ MENU
- \_\_\_\_ MENU RESET
- \_\_\_\_ MENU ON
- \_\_\_\_ MENU OFF
- \_\_\_\_ MENU SOP
- \_\_\_\_ MENU()
- DONE MOUSE ON
- DONE MOUSE OFF
- \_\_\_\_ MOUSE STOP
- DONE MOUSE()
- \_\_\_\_ OBJECT.AX
- \_\_\_\_ OBJECT.AY
- \_\_\_\_ OBJECT.CLOSE
- \_\_\_\_ OBJECT.HIT
- \_\_\_\_ OBJECT.OFF
- \_\_\_\_ OBJECT.ON
- \_\_\_\_ OBJECT.PRIORITY
- \_\_\_\_ OBJECT.SHAPE
- \_\_\_\_ OBJECT.START
- \_\_\_\_ OBJECT.STOP
- \_\_\_\_ OBJECT.VX
- \_\_\_\_ OBJECT.VX()
- \_\_\_\_ OBJECT.VY
- \_\_\_\_ OBJECT.VY()
- \_\_\_\_ OBJECT.X
- \_\_\_\_ OBJECT.X()
- \_\_\_\_ OBJECT.Y
- \_\_\_\_ OBJECT.Y()
- \_\_\_\_ ON COLLISION
- \_\_\_\_ ON MENU
- DONE ON MOUSE
- DONE ON TIMER
- DONE PAINT
- DONE PALETTE
- DONE PATTERN
- DONE POINT
- DONE POS
- \_\_\_\_ PRESET
- DONE PSET
- \_\_\_\_ PTAB
- DONE PUT [STEP]
- \_\_\_\_ SAY
- DONE SCREEN
- DONE SCREEN CLOSE
- \_\_\_\_ SCROLL
- DONE SLEEP
- DONE SOUND
- DONE SOUND WAIT
- \_\_\_\_ SOUND RESUME
- \_\_\_\_ STICK()
- \_\_\_\_ STRIG()
- DONE TIMER ON
- DONE TIMER OFF
- \_\_\_\_ TIMER STOP
- \_\_\_\_ TRANSLATE$()
- DONE WAVE
- \_\_\_\_ WIDTH LPRINT
- DONE WINDOW
- DONE WINDOW CLOSE
- DONE WINDOW OUTPUT
- DONE WINDOW()

## Examples / Demos / Libraries

- include examples from blackborn66 https://github.com/blackborn66/aqb-tests-demos

- EGads

- Benchmarks: sieve, fractals

- AMIGA hand

- ISO game engine

- BASICPaint

- Linked List

- Function Plotter
    * 2D
    * 3D

- CCGames

- M&T Book Examples

- AMOS / Blitz Libraries

