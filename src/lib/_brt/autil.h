#ifndef HAVE_AUTIL_H
#define HAVE_AUTIL_H

#include <exec/types.h>

APTR allocate_(ULONG size, ULONG flags);

void _autil_init(void);
void _autil_shutdown(void);

// BASIC error handling

#define AE_WIN_OPEN 1

extern USHORT g_errcode;

void _autil_exit(LONG return_code); // implemented in startup.s

void delay(ULONG seconds);

void _aqb_assert (BOOL b, const char *msg);

FLOAT __aqb_timer_fn (void);

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

