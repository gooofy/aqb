# AQB: Amiga QuickBasic Compiler

![AQB IDE Screenshot](doc/screenshot.png?raw=true "AQB IDE")

An experiment in alternate history: what AmigaBASIC could have looked like, had it been developed further
tailored to the Amiga OS.

Improvements over AmigaBASIC include:

* Advanced type system (including UDTs and Pointers, see below)
* Support for non-static functions and subs (enables recursion)
* Module support (similar to UNITs in TurboPascal, with full type safety and dependencies)
* Modern syntax inspired by FreeBASIC and VisualBASIC
* True native 68k compiler
* Integrated IDE besides compiler command line interface with
    * syntax highlighting
    * auto-indent
    * folding support

[AQB Command Reference](doc/ref.adoc)

Originally based on

https://github.com/ComMouse/tiger-compiler

## Requirements

* 3 MB RAM
* OS 2.0 (V36) or newer

## Installation

tbd

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

## Benchmark Results

Measured on an A500 configuration (PAL 68000, 3MB RAM) in FS-UAE, Kickstart 1.3

| Benchmark            | AmigaBasic    | GFA Basic 3.52 | BlitzBasic 2.15 | HiSoft Basic 2 | AQB    |
| -------------------- | -------------:| --------------:| ---------------:| --------------:| ------:|
| ctHLBench integer    | 33.94s        | 7.40s          | 6.96s           | 12.41s         | 1.66s  |
| ctHLBench real       | 23.90s        | 6.88s          | 4.99s           | 4.46s          | 3.12s  |
| fibonacci            | no recursion  | 54.60s         | guru            | 28.18          | 4.09s  |

## Source Code

https://github.com/gooofy/aqb

