# AQB: Amiga QuickBasic Compiler

An experiment in alternate history: what AmigaBASIC could have looked like, had it been developed further
tailored to the Amiga OS.

Originally based on

https://github.com/ComMouse/tiger-compiler

## Benchmark Results

Measured on an A500 configuration (PAL 68000, 3MB RAM) in FS-UAE, Kickstart 1.3

| Benchmark            | AmigaBasic    | GFA Basic 3.52 | BlitzBasic 2.15 | HiSoft Basic 2 | AQB    |
| -------------------- | -------------:| --------------:| ---------------:| --------------:| ------:|
| ctHLBench integer    | 33.94s        | 7.40s          | 6.96s           | 12.41s         | 1.66s  |
| ctHLBench real       | 23.90s        | 6.88s          | 4.99s           | 4.46s          | 3.12s  |
| fibonacci            | no recursion  | 54.60s         | guru            | 28.18          | 4.09s  |

## Type System

### Basic types:
* Byte, UByte (8 bits)
* Integer, UInteger (16 bits)
* Long, ULong (32 bits)
* Single (32 Bit FFP floats)
* Pointers

### Advanced types

* Arrays
* UDTs (structs)
* Function/Sub pointers
* Strings (0-terminated pointers to UByte, C-compatible)

