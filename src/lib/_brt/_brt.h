#ifndef HAVE_BRT_H
#define HAVE_BRT_H

#include <exec/types.h>
#include <clib/dos_protos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct MathBase      *MathBase;
extern struct MathTransBase *MathTransBase;

APTR   ALLOCATE_   (ULONG size, ULONG flags);
void   DEALLOCATE  (APTR ptr, ULONG size);
ULONG  FRE_        (SHORT x);

void   POKE        (ULONG adr, UBYTE  b);
void   POKEW       (ULONG adr, USHORT w);
void   POKEL       (ULONG adr, ULONG  l);

UBYTE  PEEK_       (ULONG adr);
USHORT PEEKW_      (ULONG adr);
ULONG  PEEKL_      (ULONG adr);

void _autil_init(void);
void _autil_shutdown(void);

// BASIC error handling, utils

void _debug_puts  (const UBYTE *s);
void _debug_puts2 (SHORT s);
void _debug_putu4 (ULONG l);
void _debug_putf  (FLOAT f);
void _debug_putnl (void);
void _debug_cls   (void);

//extern USHORT g_errcode;

#define ERR_OUT_OF_DATA              4
#define ERR_ILLEGAL_FUNCTION_CALL    5
#define ERR_OUT_OF_MEMORY            7
#define ERR_SUBSCRIPT_OUT_OF_RANGE   9
#define ERR_INCOMPATIBLE_ARRAY      10

void ERROR        (SHORT errcode);
void _autil_exit  (LONG return_code); // implemented in startup.s

void _cshutdown   (LONG return_code, UBYTE *msg); // implemented in cstartup.c

void _aqb_assert  (BOOL b, const UBYTE *msg);

FLOAT TIMER_      (void);

void ON_EXIT_CALL (void (*cb)(void));
void RESUME_NEXT  (void);

void SYSTEM       (void);

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

FLOAT mod_  (FLOAT divident, FLOAT divisor);

SHORT fix_  (FLOAT f);
SHORT int_  (FLOAT f);
SHORT cint_ (FLOAT f);
LONG  clng_ (FLOAT a);

FLOAT __aqb_shl_single(FLOAT a, FLOAT b);
FLOAT __aqb_shr_single(FLOAT a, FLOAT b);

/*
 * string handling
 */

void _astr_init           (void);

void _astr_itoa_ext       (LONG num, UBYTE *str, LONG base, BOOL leading_space);
void _astr_itoa           (LONG num, UBYTE *str, LONG base);
void _astr_utoa           (ULONG num, UBYTE* str, ULONG base);
void _astr_ftoa           (FLOAT value, UBYTE *buf);

UBYTE *_astr_dup          (const UBYTE *str);
SHORT __astr_cmp          (const UBYTE* s1, const UBYTE* s2);
const UBYTE *_astr_strchr (const UBYTE *s, UBYTE c);

ULONG  LEN_               (const UBYTE *str);
UBYTE  *CHR_              (LONG codepoint);
SHORT  ASC_               (const UBYTE *str);

/*
 * STR$ support
 */

UBYTE *_s1toa_   (BYTE   b);
UBYTE *_s2toa_   (SHORT  i);
UBYTE *_s4toa_   (LONG   l);
UBYTE *_u1toa_   (UBYTE  b);
UBYTE *_u2toa_   (USHORT i);
UBYTE *_u4toa_   (ULONG  l);
UBYTE *_ftoa_    (FLOAT  f);
UBYTE *_booltoa_ (BOOL   b);

/*
 * VAL* support
 */

LONG _str2i4_ (UBYTE *str, LONG len, LONG base);
FLOAT _str2f_ (UBYTE *str, LONG len, LONG base);

FLOAT  VAL_     (UBYTE *s);
SHORT  VALINT_  (UBYTE *s);
USHORT VALUINT_ (UBYTE *s);
LONG   VALLNG_  (UBYTE *s);
ULONG  VALULNG_ (UBYTE *s);

/*
 * dynamic array support
 */

typedef struct
{
    ULONG   lbound, ubound, numElements;
} _DARRAY_BOUNDS_T;

typedef struct
{
    APTR              data;
    UWORD             numDims;
    ULONG             elementSize;
    ULONG             dataSize;
    _DARRAY_BOUNDS_T *bounds;
} _DARRAY_T;

void  __DARRAY_T___init__ (_DARRAY_T *self, ULONG elementSize);
void  __DARRAY_T_REDIM    (_DARRAY_T *self, BOOL preserve, UWORD numDims, ...);
void *__DARRAY_T_IDXPTR_  (_DARRAY_T *self, UWORD dimCnt, ...);
WORD  __DARRAY_T_LBOUND_  (_DARRAY_T *self, WORD d);
WORD  __DARRAY_T_UBOUND_  (_DARRAY_T *self, WORD d);
void  __DARRAY_T_COPY     (_DARRAY_T *self, _DARRAY_T *a);
void  __DARRAY_T_ERASE    (_DARRAY_T *self);


// DATA / READ / RESTORE support

void _aqb_restore (void *p);
void _aqb_read1   (void *v);
void _aqb_read2   (void *v);
void _aqb_read4   (void *v);
void _aqb_readStr (void *v);

#endif
