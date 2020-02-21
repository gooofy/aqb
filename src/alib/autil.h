#ifndef HAVE_AUTIL_H
#define HAVE_AUTIL_H

#include <exec/types.h>

// AllocRemember wrapper

APTR ralloc(ULONG size, ULONG flags);

void autil_init(void);
void autil_shutdown(void);

// BASIC error handling

#define AE_WIN_OPEN 1

extern USHORT g_errcode;

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

