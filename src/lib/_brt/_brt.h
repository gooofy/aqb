#ifndef HAVE_BRT_H
#define HAVE_BRT_H

#include <exec/types.h>
#include <clib/dos_protos.h>

/*
 * autil
 */

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct MathBase      *MathBase;
extern struct MathTransBase *MathTransBase;

APTR allocate_(ULONG size, ULONG flags);

void _autil_init(void);
void _autil_shutdown(void);

// BASIC error handling

#define AE_WIN_OPEN 1

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
 * aio
 */

extern BPTR g_stdout;

void _aio_init(void);
void _aio_shutdown(void);

void _aio_puts4(int num);
void _aio_puts2(short num);
void _aio_puts1(char num);
void _aio_putu4(unsigned int num);
void _aio_putu2(unsigned short num);
void _aio_putu1(unsigned char num);
void _aio_puthex(int num);
void _aio_putuhex(ULONG l);
void _aio_putbin(int num);
void _aio_putf(FLOAT f);
void _aio_putbool(BOOL b);

void _aio_puts(const char *str);

void _aio_putnl(void);
void _aio_puttab(void);

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
 * astr
 */

void _astr_init(void);

void _astr_itoa(int num, char *str, int base);
void _astr_utoa(unsigned int num, char* str, unsigned int base);

ULONG len_(const char *str);

char *_astr_dup(const char *str);

void _astr_ftoa(FLOAT value, char *buf);

const char *_astr_strchr(const char *s, char c);
#endif

