#ifndef HAVE_BRT_H
#define HAVE_BRT_H

#include <exec/types.h>
#include <clib/dos_protos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct MathBase      *MathBase;
extern struct MathTransBase *MathTransBase;

APTR allocate_(ULONG size, ULONG flags);

void _autil_init(void);
void _autil_shutdown(void);

// BASIC error handling, utils

void _debug_puts (const char *s);
void _debug_puts2(SHORT s);

extern USHORT g_errcode;

void _autil_exit(LONG return_code); // implemented in startup.s

void _cshutdown (LONG return_code, char *msg); // implemented in cstartup.c

void _aqb_assert (BOOL b, const char *msg);

FLOAT __aqb_timer_fn (void);

void _aqb_on_exit_call(void (*cb)(void));

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

/*
 * amath
 */

void _amath_init(void);

FLOAT mod_(FLOAT divident, FLOAT divisor);

SHORT fix_  (FLOAT f);
SHORT int_  (FLOAT f);
SHORT cint_ (FLOAT f);
LONG  clng_ (FLOAT a);

FLOAT __aqb_shl_single(FLOAT a, FLOAT b);
FLOAT __aqb_shr_single(FLOAT a, FLOAT b);

/*
 * string handling
 */

void _astr_init(void);

void _astr_itoa(int num, char *str, int base);
void _astr_utoa(unsigned int num, char* str, unsigned int base);
void _astr_ftoa(FLOAT value, char *buf);

ULONG len_(const char *str);

char *_astr_dup(const char *str);
SHORT __astr_cmp(const char* s1, const char* s2);
const char *_astr_strchr(const char *s, char c);

/*
 * STR$ support
 */

char *_s1toa_   (BYTE   b);
char *_s2toa_   (SHORT  i);
char *_s4toa_   (LONG   l);
char *_u1toa_   (UBYTE  b);
char *_u2toa_   (USHORT i);
char *_u4toa_   (ULONG  l);
char *_ftoa_    (FLOAT  f);
char *_booltoa_ (BOOL   b);

#endif
