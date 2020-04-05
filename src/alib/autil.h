#ifndef HAVE_AUTIL_H
#define HAVE_AUTIL_H

#include <exec/types.h>

// AllocRemember wrapper

APTR _autil_alloc(ULONG size, ULONG flags);

void _autil_init(void);
void _autil_shutdown(void);

// BASIC error handling

#define AE_WIN_OPEN 1

extern USHORT g_errcode;

void _autil_exit(LONG return_code); // implemented in startup.s

void delay(ULONG seconds);


// unit testing utils

void assertTrue (BOOL b, const char *msg);
void assertEqualsInt    (SHORT a, SHORT b, const char *msg);
void assertEqualsLong   (LONG  a, LONG  b, const char *msg);
void assertEqualsSingle (FLOAT a, FLOAT b, const char *msg);

#if 0  // exec has these already

#define BOOL char

#define TRUE   1
#define FALSE  0

#define BYTE   char
#define UBYTE  unsigned char

#define SHORT  short
#define USHORT unsigned short

#define LONG   long
#define ULONG  unsigned long

#endif

#endif

