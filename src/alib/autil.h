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

void _autil_exit(void); // implemented in startup.s

void _autil_sleep(ULONG seconds);

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

