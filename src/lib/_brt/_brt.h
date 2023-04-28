#ifndef HAVE_BRT_H
#define HAVE_BRT_H

#include <exec/types.h>
#include <clib/dos_protos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct MathBase      *MathBase;
extern struct MathTransBase *MathTransBase;

APTR   ALLOCATE_   (ULONG size, ULONG flags);
void   DEALLOCATE  (APTR ptr);
void   _MEMSET     (BYTE *dst, BYTE c, ULONG n);
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

void _debug_putc   (const char c);
void _debug_puts   (const UBYTE *s);
void _debug_puts1  (BYTE s);
void _debug_puts2  (SHORT s);
void _debug_puts4  (LONG l);
void _debug_putu1  (UBYTE num);
void _debug_putu2  (UWORD num);
void _debug_putu4  (ULONG num);
void _debug_puthex (ULONG num);
void _debug_putf   (FLOAT f);
void _debug_puttab (void);
void _debug_putnl  (void);
void _debug_cls    (void);
void _debug_break  (void);

// #define ENABLE_DPRINTF

void dprintf(const char *format, ...);

#ifdef ENABLE_DPRINTF

#define DPRINTF(...) dprintf(__VA_ARGS__)


#else

#define DPRINTF(...)

#endif

#define ERR_OUT_OF_DATA                4
#define ERR_ILLEGAL_FUNCTION_CALL      5
#define ERR_OUT_OF_MEMORY              7
#define ERR_SUBSCRIPT_OUT_OF_RANGE     9
#define ERR_INCOMPATIBLE_ARRAY        10

#define ERR_BAD_FILE_NUMBER           52
#define ERR_BAD_FILE_MODE             54
#define ERR_IO_ERROR                  57
#define ERR_BAD_FILE_NAME             64

void   _aqb_assert   (BOOL b, const UBYTE *msg);
void   ERROR         (SHORT errcode);
void   RESUME_NEXT   (void);
void   ON_ERROR_CALL (void (*cb)(void));
void   ON_EXIT_CALL  (void (*cb)(void));
void   ON_BREAK_CALL (void (*cb)(void));
void   _autil_exit   (LONG return_code); // implemented in startup.s

void   _cshutdown    (LONG return_code, UBYTE *msg); // implemented in cstartup.c

FLOAT  TIMER_        (void);
STRPTR DATE_         (void);
void   SLEEP_FOR     (FLOAT s);

void   SYSTEM        (void);

// program startup mode / debugger connection

#define STARTUP_CLI    1    // started from shell/cli
#define STARTUP_WBENCH 2    // started from workbench
#define STARTUP_DEBUG  3    // started from debugger/ide

extern USHORT   _startup_mode;
extern BOOL     _do_resume;     // set by RESUME NEXT

#define DEBUG_SIG 0xDECA11ED

#define DEBUG_CMD_START    23
#define DEBUG_CMD_PUTC     24
#define DEBUG_CMD_PUTS     25

/* debugger sends this instead of WBStartup */
struct DebugMsg
{
    struct Message  msg;
    struct MsgPort *port;
    ULONG           debug_sig;					// 24
    UWORD           debug_cmd;					// 28
    ULONG           debug_exitFn;               // 30
    union
    {
        ULONG   err;    // START return msg		// 34
        char    c;      // putc					// 34
        char   *str;    // puts					// 34
    }u;
};

extern struct DebugMsg *__StartupMsg;

// stack swap support

extern ULONG                   _aqb_stack_size;
extern ULONG                  *_g_stack;

// CTRL-C / CTRL-D (DEBUG) BREAK handling

#define BREAK_CTRL_C    1
#define BREAK_CTRL_D    2

extern USHORT _break_status;

#define CHKBRK if (_break_status) __handle_break()

void __handle_break(void);

//void *memset (void *dst, register int c, register int n);

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

//typedef long int SFVALUE;
#ifndef SItype
#define SItype long int
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
void _astr_utoa_ext       (ULONG num, UBYTE* str, ULONG base, BOOL leading_space);
void _astr_ftoa           (FLOAT value, UBYTE *buf);
void _astr_ftoa_ext       (FLOAT value, UBYTE *buf, BOOL leading_space);

UBYTE *_astr_dup          (const UBYTE *str);
SHORT __astr_cmp          (const UBYTE* s1, const UBYTE* s2);
const UBYTE *_astr_strchr (const UBYTE *s, UBYTE c);
UBYTE *__astr_concat      (const UBYTE *a, const UBYTE *b);

ULONG  LEN_               (const UBYTE *str);
UBYTE  *CHR_              (LONG codepoint);
SHORT  ASC_               (const UBYTE *str);
UBYTE  *MID_              (const UBYTE *str, SHORT n, SHORT m);
UBYTE *UCASE_             (const UBYTE *s);
UBYTE *LCASE_             (const UBYTE *s);
UBYTE *LEFT_              (const UBYTE *s, SHORT n);
UBYTE *RIGHT_             (const UBYTE *s, SHORT n);
SHORT  INSTR              (SHORT n, const UBYTE *x, const UBYTE *y);

/*
 * utils
 */

extern struct Task  *_autil_task;

struct MsgPort   *_autil_create_port   (STRPTR name, LONG pri);
void              _autil_delete_port   (struct MsgPort *port);
struct IORequest *_autil_create_ext_io (struct MsgPort *port, LONG iosize);
struct IOStdReq  *_autil_create_std_io (struct MsgPort *port);
void              _autil_delete_ext_io (struct IORequest *ioreq);
void              _autil_begin_io      (struct IORequest *iorequest);

#define NEWLIST(l) ((l)->lh_Head = (struct Node *)&(l)->lh_Tail, \
                    (l)->lh_Tail = NULL, \
                    (l)->lh_TailPred = (struct Node *)&(l)->lh_Head)

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
 * OOP
 */

typedef struct CObject_ CObject;

struct CObject_
{
    VOID   ***_vTablePtr;
};

STRPTR _CObject_ToString_ (CObject *THIS);
BOOL   _CObject_Equals_ (CObject *THIS, CObject *obj);
ULONG  _CObject_GetHashCode_ (CObject *THIS);

/*
 * dynamic array support
 */

typedef struct CArray_ CArray;
typedef struct CArrayBounds_ CArrayBounds;
typedef struct CArrayEnumerator_ CArrayEnumerator;

struct CArray_
{
    intptr_t **_vTablePtr;
    intptr_t **__intf_vtable_IList;
    intptr_t **__intf_vtable_ICollection;
    intptr_t **__intf_vtable_IEnumerable;
    intptr_t **__intf_vtable_ICloneable;
    BYTE    *_data;
    UWORD    _numDims;
    LONG     _elementSize;
    LONG     _dataSize;
    CArrayBounds *_bounds;
};

struct CArrayBounds_
{
    LONG     lbound;
    LONG     ubound;
    LONG     numElements;
};

struct CArrayEnumerator_
{
    intptr_t **_vTablePtr;
    intptr_t **__intf_vtable_IEnumerator;
    CArray *_array;
    LONG     _index;
    intptr_t _currentElement;
};

VOID        _CArrayEnumerator___init      (CArrayEnumerator *THIS);
VOID        _CArrayEnumerator_CONSTRUCTOR (CArrayEnumerator *THIS, CArray *list);
BOOL        _CArrayEnumerator_MoveNext_   (CArrayEnumerator *THIS);
intptr_t    _CArrayEnumerator_Current_    (CArrayEnumerator *THIS);
VOID        _CArrayEnumerator_Reset       (CArrayEnumerator *THIS);

void        _CArray___init                (CArray *THIS);
VOID        _CArray_CONSTRUCTOR           (CArray *THIS, LONG     elementSize);
VOID        _CArray_REDIM                 (CArray *THIS, UWORD numDims, BOOL preserve, ...);
intptr_t   *_CArray_IDXPTR_               (CArray *THIS, UWORD    dimCnt, ...);
LONG        _CArray_LBOUND_               (CArray *THIS, WORD     d);
LONG        _CArray_UBOUND_               (CArray *THIS, WORD     d);
VOID        _CArray_COPY                  (CArray *THIS, CArray *darray);
VOID        _CArray_ERASE                 (CArray *THIS);
LONG        _CArray_Count_                (CArray *THIS);
VOID        _CArray_capacity              (CArray *THIS, LONG     c);
LONG        _CArray_capacity_             (CArray *THIS);
intptr_t    _CArray_GetAt_                (CArray *THIS, LONG     index);
VOID        _CArray_SetAt                 (CArray *THIS, LONG     index, intptr_t obj);
intptr_t ***_CArray_GetEnumerator_        (CArray *THIS);
LONG        _CArray_Add_                  (CArray *THIS, intptr_t obj);
BOOL        _CArray_Contains_             (CArray *THIS, intptr_t value);
VOID        _CArray_CLEAR                 (CArray *THIS);
BOOL        _CArray_IsReadOnly_           (CArray *THIS);
BOOL        _CArray_IsFixedSize_          (CArray *THIS);
LONG        _CArray_IndexOf_              (CArray *THIS, intptr_t value, LONG     startIndex, LONG     Count);
VOID        _CArray_Insert                (CArray *THIS, LONG     index, intptr_t value);
VOID        _CArray_Remove                (CArray *THIS, intptr_t value);
VOID        _CArray_RemoveAt              (CArray *THIS, LONG     index);
VOID        _CArray_RemoveAll             (CArray *THIS);
CObject    *_CArray_Clone_                (CArray *THIS);

// DATA / READ / RESTORE support

void _aqb_restore (void *p);
void _aqb_read1   (void *v);
void _aqb_read2   (void *v);
void _aqb_read4   (void *v);
void _aqb_readStr (void *v);

/*
 * PRINT / INPUT / terminal statement support
 */

typedef BOOL (*_aio_puts_cb_t)        (UBYTE *s);
typedef BOOL (*_aio_puttab_cb_t)      (void);
typedef BOOL (*_aio_gets_cb_t)        (UBYTE *buf, USHORT buf_len, BOOL do_nl);
typedef BOOL (*_aio_cls_cb_t)         (void);
typedef BOOL (*_aio_locate_cb_t)      (SHORT l, SHORT c);
typedef void (*_autil_sleep_for_cb_t) (FLOAT s);

extern _aio_puts_cb_t        _aio_puts_cb;
extern _aio_gets_cb_t        _aio_gets_cb;
extern _aio_cls_cb_t         _aio_cls_cb;
extern _aio_locate_cb_t      _aio_locate_cb;
extern _autil_sleep_for_cb_t _autil_sleep_for_cb;

void _aio_init                   (void);
void _aio_shutdown               (void);

void _aio_puts4                  (USHORT fno, LONG num);
void _aio_puts2                  (USHORT fno, SHORT num);
void _aio_puts1                  (USHORT fno, UBYTE num);
void _aio_putu4                  (USHORT fno, ULONG num);
void _aio_putu2                  (USHORT fno, USHORT num);
void _aio_putu1                  (USHORT fno, UBYTE num);
void _aio_puthex                 (USHORT fno, LONG num);
void _aio_putuhex                (USHORT fno, ULONG l);
void _aio_putbin                 (USHORT fno, LONG num);
void _aio_putf                   (USHORT fno, FLOAT f);
void _aio_putbool                (USHORT fno, BOOL b);

void _aio_puts                   (USHORT fno, const UBYTE *str);

void _aio_putnl                  (USHORT fno);
void _aio_puttab                 (USHORT fno);

struct FileHandle *_aio_getfh    (USHORT fno);

void _aio_writes4                (USHORT fno, LONG num);
void _aio_writes2                (USHORT fno, SHORT num);
void _aio_writes1                (USHORT fno, UBYTE num);
void _aio_writeu4                (USHORT fno, ULONG num);
void _aio_writeu2                (USHORT fno, USHORT num);
void _aio_writeu1                (USHORT fno, UBYTE num);
void _aio_writef                 (USHORT fno, FLOAT f);
void _aio_writebool              (USHORT fno, BOOL b);
void _aio_writes                 (USHORT fno, const UBYTE *str);
void _aio_writecomma             (USHORT fno, BOOL b);

// [ LINE ] INPUT support:

void _aio_line_input             (USHORT fno, UBYTE *prompt, UBYTE **s, BOOL do_nl);
void _aio_console_input          (BOOL qm, UBYTE *prompt, BOOL do_nl);
void _aio_inputs1                (USHORT fno, BYTE   *v);
void _aio_inputu1                (USHORT fno, UBYTE  *v);
void _aio_inputs2                (USHORT fno, SHORT  *v);
void _aio_inputu2                (USHORT fno, USHORT *v);
void _aio_inputs4                (USHORT fno, LONG   *v);
void _aio_inputu4                (USHORT fno, ULONG  *v);
void _aio_inputf                 (USHORT fno, FLOAT  *v);
void _aio_inputs                 (USHORT fno, UBYTE  **v);
void _aio_set_dos_cursor_visible (BOOL visible);

void  LOCATE                     (SHORT l, SHORT c);
void  CLS                        (void);

#define FILE_MODE_RANDOM      0
#define FILE_MODE_INPUT       1
#define FILE_MODE_OUTPUT      2
#define FILE_MODE_APPEND      3
#define FILE_MODE_BINARY      4

#define FILE_ACCESS_READ      0
#define FILE_ACCESS_WRITE     1
#define FILE_ACCESS_READWRITE 2

void _aio_open  (UBYTE *fname, USHORT mode, USHORT access, USHORT fno, USHORT recordlen);
void _aio_close (USHORT fno);

BOOL EOF_       (USHORT fno);

#endif
