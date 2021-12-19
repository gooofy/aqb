## 0.8.1

Improvements:

    * runtime: SPRITE() function added
    * runtime: SPRITE SHOW command added
    * runtime: SPRITE MOVE command added
    * runtime: SPRITE FREE command added
    * runtime: ILBM LOAD SPRITE command added
    * runtime: POINTER SPRITE command added
    * runtime: POINTER CLEAR command added

Bug Fixes:



## 0.8.0

Improvements:

    * Major new feature: source level debugging

    * ide: debug mode mode plus output added
    * ide: debug/console view added
    * ide: SMART\_REFRESH window for faster+reliable refresh handling
    * ide: editor breakpoints (F9)
    * compiler: OPTION DEBUG statement added
    * compiler: TRACE statement added
    * compiler: BREAK statement added
    * compiler: generate debug info
    * debugger: exit or continue choice on traps, runtime errors and debug breaks
    * debugger: display stacktrace, source line and registers
    * runtime: CTRL-C will issue a DEBUG BREAK when debugger is active
    * runtime: BITMAP\_t supports continous plane allocation suitable for BOB use now
    * runtime: BITMAP OUTPUT command added
    * runtime: VWAIT command added
    * runtime: LOCATE XY command added
    * runtime: CIRCLE command added
    * runtime: COLOR command: optional draw mode argument added
    * runtime: new AnimSupport module added
    * runtime: GELS INIT command added
    * runtime: GELS REPAINT command added
    * runtime: BOB() function added
    * runtime: BOB MOVE command added
    * runtime: BOB SHOW command added
    * runtime: BOB HIDE command added
    * runtime: ILBM LOAD BOB command added
    * MIT license applied consistently

Bug Fixes:

    * runtime: INPUT statement fixed
    * build system: makefile portability enhancements
    * build system: fix library link order (J. Becher)
    * compiler: fix BYREF recursive argument passing
    * compiler: fix BYREF argument assignment
    * compiler: fix IF/ELSE namespaces
    * compiler: fix BYTE/UBYTE/UINTEGER to SINGLE conversion
    * linker: handle duplicate symbols gracefully
    * ide: fix run state display

## 0.7.3

Improvements:

    * ide/runtime: handle and display program exit ERR code
    * ide: keep ASL file requester path
    * ide: fix multiview (help viewer) wb startup
    * runtime: IFFSupport module added
    * runtime: BLIT() function added
    * runtime: BLIT FREE instruction added
    * runtime: GET instruction added
    * runtime: PUT instruction added
    * runtime: ALLOCATE, DEALLOCATE, \_MEMSET added
    * runtime: PALETTE LOAD instruction added
    * runtime: SCREEN/WINDOW: use original graphics/intuition flags
    * runtime: INKEY$: no implicit sleep
    * compiler: add generic #fno syntax

Bug Fixes:

    * ide: fix cursor line in buf2line()
    * compiler: fix subprogram call error handling
    * compiler: fix record/class field memory offsets
    * compiler: fix module type serialization
    * compiler: detect multiple declarations of same constant identifier

## 0.7.2

Improvements:

    * documentation: full online documentation in amiga guide format
    * ide: make editor auto-formatting much less aggressive (respect user's whitespace and case choices)
    * runtime: ON MOUSE CALL instruction added
    * runtime: MOUSE ON|OFF instruction added
    * runtime: MOUSE() function added
    * runtime: ON MOUSE MOTION CALL instruction added
    * runtime: MOUSE MOTION ON|OFF instruction added

Bug Fixes:

    * ide: fix changed marker position
    * ide: keep DOS Output() when executing program from within IDE
    * ide: fix line length check on cursor up/down
    * ide: fix vertical scroll area
    * ide: fix run memleak
    * ide: fix UI slowdown bug (added proper Begin/EndRefresh calls)
    * compiler: fix EOL token leak, REM comment processing

## 0.7.1alpha1

Improvements:

    * runtime/ide: detect and handle CTRL-C breaks
    * ide: handle workbench startup + argument
    * ide: create icons for source code files and binaries
    * compiler/runtime: String concatenation implemented
    * compiler: SLEEP FOR statement added
    * runtime: workbench startup code added
    * runtime: auto-open WINDOW 1 on first text output in debug/worbench startup mode

Bug Fixes:

    * compiler: CLI return code fixed
    * compiler: handle varPtr in BinOp expressions
    * compiler: reset compiler options on each compiler run
    * ide: do not crash if non-existing file is specified as cmd line argument
    * ide: fix refresh while program is running
    * runtime: fix \_aqb shutdown crash
    * runtime: fix INKEY buffer handling
    * runtime: fix TIMER nullpointer access

## 0.7.0alpha2

Improvements:

    * "Run" menu added
    * PATTERN RESTORE (ACE) statement added

Bug Fixes:

    * compiler: for loop floating point step fixed
    * compiler: SINGLE -> INTEGER const conversion fixed
    * runtime: restore all non-scratch registers on premature program end (END / EXIT statements)
	* ide: SELECT CASE autoformat fixed
    * ide: Editor line join folding fixed
    * ide: Preserve floating point literals when auto-formatting
    * examples: 3dplot fixed

## 0.7.0alpha1

