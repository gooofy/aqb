# AQB: A BASIC Compiler and IDE for Amiga Computers

![Screenshot](https://raw.githubusercontent.com/gooofy/aqb/master/doc/screenshot.png)

### About

* [Project Scope](#project-scope)
* [Requirements](#requirements)
* [Installation](#installation)
* [Benchmark Results](#benchmark-results)
* [Source Code](#source-code)

### AQB Programming Language
* [Language reference: Core](help/RefCore.md)
* [Language reference: Amiga specific commands](help/RefAmiga.md)
* [Type System](#type-system)
* [Module System and Runtime](#module-system-and-runtime)
* [Code Generation and Target Systems](#code-generation-and-target-systems)
* [Interrupting / break handling in AQB programs](#interrupting--break-handling-in-aqb-programs)
* [Amiga OS System Programming in AQB](#amiga-os-system-programming-in-aqb)

### IDE

* [Keyboard shortcuts](#keyboard-shortcuts)

## Project Scope

An experiment in alternate history: what AmigaBASIC could have looked like,
had it been developed further tailored to the Amiga OS.

What AQB is not: AQB does not try to be a clone of any particular BASIC
dialect - neither QuickBASIC, FreeBASIC or VisualBASIC nor any particular
Amiga specific BASIC implementation like AmigaBASIC, ACE, HiSoft, GFA,
Blitz or AMOS.  While it strives to be as compatible as possible with the
Microsoft BASIC family of languages (and certainly has many QuickBASIC
traits) the primary focus is on the creation of a modern, clean, Amiga
OS-compliant, future-proof BASIC that is tailored towards modern Amiga
application development.

To be more specific, FreeBASIC is the source of many core AQB language
constructs (in many respects AQB can be considered a subset of FreeBASIC)
with inspiration for Amiga specific commands mainly from AmigaBASIC, ACE
and HiSoft.  Main target is Amiga OS compliant application development.

Improvements over AmigaBASIC include:

* Advanced type system (including UDTs and Pointers, see below)
* Support for non-static functions and subs (enables recursion)
* Module support (similar to UNITs in TurboPascal, with full type safety
  and dependencies)
* Modern syntax inspired by FreeBASIC and VisualBASIC
* True native 68k compiler
* Integrated IDE besides compiler command line interface with
    * syntax highlighting
    * auto-indent
    * folding support

## Requirements

* 3 MB RAM
* OS 2.0 (V36) or newer

## Installation

Download a release LHA archive (https://github.com/gooofy/aqb/releases) and
unpack it wherever you like, keep the directory structure intact.

AQB should run from this point on without the need for further installation,
but for convenience add a "AQB:" assign to your `S:user-startup` file, e.g.

;BEGIN AQB
Assign AQB: "sys:Apps/AQB"
;END AQB

## Type System

### Basic types:
* Byte, UByte (8 bits)
* Integer, UInteger (16 bits)
* Long, ULong (32 bits)
* Single (32 Bit FFP floats)

### Advanced types

* Static (C-like, fast) and dynamic (runtime bounds checking) arrays
* UDTs (structs)
* OOP (FreeBASIC like)
* Pointers (C-like, including function/sub pointers)
* Strings (0-terminated pointers to UByte, C-compatible)

## Module System and Runtime

AQB tries to keep the set of commands that are built into the compiler to a
minimum and relies on its quite powerful module system to provide most of
the commands as part of the runtime system. This means that while the
default runtime strives to implement a modern QuickBASIC like dialect
tailored to the Amiga, it is quite possible to implement alternative
runtime modules that could provide AQB with a different "personality", e.g.
one that is closer to AmigaBASIC or GFA BASIC or even languages like
BlitzBasic ot AMOS.

The goal for AQB's default runtime is to provide a rich set of commands
covering typical Amiga OS programming topics like GUI programming,
multitasking, graphics and audio combined with resource tracking and
error/exception handling. Future plans might also include an automated
garbage collector to make memory allocation easier and safer.

AQB is fully link-compatible with the Amiga 68k GCC compiler which means
that AQB modules can be implemented in C as well as BASIC (one could even
mix these languages within one module, i.e. implement some subprograms in
C while others in BASIC).

### Intuition / Exec event handling

Since the default runtime wants to enable OS friendly programming no busy
waiting is used. Therefore the SLEEP command is used to process pending
events, i.e. you will need to call SLEEP regularly in your program,
typically form a main loop that could look like this:

    WHILE running
        SLEEP
    WEND

For event processing you register subroutines using the ON ... CALL
<function> family of statements, e.g.

    ON WINDOW CALL myWindowHandler

see https://github.com/gooofy/aqb/blob/master/examples/demo/gfx1.bas for a
simple example of this approach.

Interesting detail: since AQB supports C-like function pointers, the ON ...
CALL family of statements is not built into the compiler but part of the
`_aqb` runtime:

    PUBLIC DECLARE SUB ON WINDOW CALL (BYVAL p AS SUB)

## Code Generation and Target Systems

At the time of this writing classic 68k Amiga systems is the only compiler
target. The idea is to focus on one target and try to make AQB work really
well on this platform before expanding to other systems. The AQB compiler
is implemented from scratch in C based on Appel's 1997 book "Modern
Compiler Implementation in C" and tries to keep system requirements (RAM
and CPU) low while still producing somewhat sensible machine code.
Originally the AQB code was based on ComMouses's tiger compiler
implementation (https://github.com/ComMouse/tiger-compiler) which provided
a very useful starting point.

For future expansions to other platforms the current plan is to use an LLVM
based backend for all platforms powerful enough to run LLVM which is
probably true for most NG Amiga systems (AROS, AmigaOS 4 and MorphOS) and
most likely also for highly expanded classic Amiga systems (using
accelerator cards such as PiStom or Vampire).

As for the 68k compiler future plans include further reduction of its
memory footprint ideally to a point where it is useful on 1MB or even 512K
Amiga systems. At that point it might even make sense to implement a 6502
backend targeting modern 8 bit systems like the MEGA65, Commander X16 or
C256 Foenix.

## Interrupting / break handling in AQB programs

By default, the runtime will check for break signals in i/o routines. Break
signals can be sent either by pressing CTRL-C or by using the AmigaDOS
BREAK command.

Additionally, the compiler will insert checks for break signals in all loop
statements (to protect against endless loops) and in subprogram startup
code (to protect against infinite recursion). This is enabled by default
but since this will make code longer and cost some performance, it can be
switched off using the

    OPTION BREAK OFF

statement.

## Amiga OS System Programming in AQB

AQB datatypes are very similar to C (C-like strings, structs and pointers)
which makes usage of Amiga OS libraries and devices pretty seamless.

Data structures generally can be modeled 1:1 from their C counterparts, a
python script semi-automating the task of converting Amiga C library and
device headers to AQB is in the works. Here is a preview of what the
resulting AQB declarations typically look like:

    [...]

    TYPE ViewPort
        AS ViewPort PTR NextViewPort
        AS ColorMap PTR ColorMap
        AS CopList PTR DspIns, SprIns, ClrIns
        AS UCopList PTR UCopIns
        AS INTEGER DWidth, DHeight, DxOffset, DyOffset
        AS UINTEGER Modes
        AS UBYTE SpritePriorities, ExtendedModes
        AS RasInfo PTR RasInfo
    END TYPE

    TYPE Layer_Info
        AS Layer PTR top_layer, check_lp
        AS ClipRect PTR obs, FreeClipRects
        AS LONG PrivateReserve1, PrivateReserve2
        AS SignalSemaphore Lock
        AS MinList gs_Head
        AS INTEGER PrivateReserve3
        AS VOID PTR PrivateReserve4
        AS UINTEGER Flags
        AS BYTE fatten_count, LockLayersCount
        AS INTEGER PrivateReserve5
        AS VOID PTR BlankHook, LayerInfo_extra
    END TYPE

    EXTERN GfxBase AS VOID PTR

    DECLARE SUB Move (rp AS RastPort PTR, x AS INTEGER, y AS INTEGER) LIB -240 GfxBase (a1, d0, d1)
    DECLARE SUB RectFill (rp AS RastPort PTR, xmin AS INTEGER, ymin AS INTEGER, xmax AS INTEGER, ymax AS INTEGER) LIB -306 GfxBase (a1, d0, d1, d2, d3)
    DECLARE SUB Draw (rp AS RastPort PTR, x AS INTEGER, y AS INTEGER) LIB -246 GfxBase (a1, d0, d1)
    DECLARE SUB SetAPen (rp AS RastPort PTR, pen AS INTEGER) LIB -342 GfxBase (a1, d0)

    [...]

## Keyboard shortcuts:
    * F1     - this help screen
    * S-UP   - page up
    * S-DOWN - page down
    * Ctrl-T - goto top of file
    * Ctrl-B - goto end of file
    * Ctrl-Y - delete line
    * F5     - compile & run
    * F7     - compile
    * Ctrl-F - find
    * Ctrl-N - find next
    * Ctrl-M - mark block
    * Ctrl-S - save
    * Ctrl-C - quit

## Benchmark Results

Measured on an A500 configuration (PAL 68000, 3MB RAM) in FS-UAE, Kickstart 1.3

| Benchmark            | AmigaBasic    | GFA Basic 3.52 | BlitzBasic 2.15 | HiSoft Basic 2 | AQB   |
| -------------------- | ----------    | -------------- | --------------- | -------------- | ----- |
| ctHLBench integer    | 33.94s        | 7.40s          | 6.96s           | 12.41s         | 1.66s |
| ctHLBench real       | 23.90s        | 6.88s          | 4.99s           | 4.46s          | 3.12s |
| fibonacci            | no recursion  | 54.60s         | guru            | 28.18          | 4.09s |

## Source Code

https://github.com/gooofy/aqb

