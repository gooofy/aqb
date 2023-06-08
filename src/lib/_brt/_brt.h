#ifndef HAVE_BRT_H
#define HAVE_BRT_H

/**********************************************************************
 **                                                                  **
 **  _brt: BASIC runtime, minimal AQB runtime module                 **
 **                                                                  **
 **********************************************************************/

#include <exec/types.h>
#include <clib/dos_protos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct MathBase      *MathBase;
extern struct MathTransBase *MathTransBase;

/********************************************************************
 *                                                                  *
 *  startup / init                                                  *
 *  --------------                                                  *
 *                                                                  *
 *  startup.S   - call _cstartup()                                  *
 *              - do a stack swap if requested                      *
 *              - call _aqb_main()                                  *
 *                                                                  *
 *  _cstartup() - open libraries                                    *
 *              - call _autil_init()                                *
 *              - setup CTRL-C handler                              *
 *              - call _astr_init(), _amath_init(), _aio_init(),    *
 *                gc_init()                                         *
 *                                                                  *
 *                                                                  *
 *  shutdown / exit                                                 *
 *  ---------------                                                 *
 *                                                                  *
 *  _autil_exit() is implemented in startup.S.                      *
 *  _autil_exit() calls _c_atexit()                                 *
 *                                                                  *
 *  _c_atexit() - run exit handlers registered via ON_EXIT_CALL()   *
 *              - remove CTRL-C handler                             *
 *              - run _aio_shutdown, _autil_shutdown                *
 *              - close libraries                                   *
 *                                                                  *
 *  _cshutdown() - print error message if possible                  *
 *               - call _autil_exit()                               *
 *                                                                  *
 *                                                                  *
 *  stack swap                                                      *
 *  ----------                                                      *
 *                                                                  *
 *  if _aqb_stack_size in nonzero, a custom stack will be allocated *
 *  for the application. AllocVec() is done in _cstartup,           *
 *  Exec's StackSwap() is then called from startup.s                *
 *                                                                  *
 *  FIXME: check for current stack size, swap stack only if         *
 *         _aqb_stack_size is larger then what we have already      *
 *                                                                  *
 ********************************************************************/

void _cstartup       (void);             // implemented in cstartup.c, called by startup.S

void _autil_init     (void);
void _amath_init     (void);
void _astr_init      (void);
void _aio_init       (void);
void _gc_init        (void);

void  _cshutdown     (LONG return_code, UBYTE *msg); // implemented in cstartup.c, calls _autil_exit()
void _autil_exit     (LONG return_code); // implemented in startup.S
void _c_atexit       (void);             // implemented in cstartup.c, called by _autil_exit()
void ON_EXIT_CALL    (void (*cb)(void)); // register exit handler to be called from _c_atexit()
void _autil_shutdown (void);
void _aio_shutdown   (void);
void _gc_shutdown    (void);

void SYSTEM          (void);             // BASIC command, calls _autil_exit(0)

// stack swap support
extern ULONG         _aqb_stack_size;
extern ULONG        *_g_stack;

// startup mode
#define STARTUP_CLI    1    // started from shell/cli
#define STARTUP_WBENCH 2    // started from workbench
#define STARTUP_DEBUG  3    // started from debugger/ide

extern USHORT   _startup_mode;

/********************************************************************
 *                                                                  *
 * CTRL-C / CTRL-D (DEBUG) BREAK handling                           *
 *                                                                  *
 * _brt installs an input handler (via input.device) which will     *
 * (just) the set _break_status global variable when a              *
 * CTRL-C/CTRL-D input event occurs.                                *
 *                                                                  *
 * the application needs to check for this condition at strategic   *
 * locations regularly and call __handle_break() or handle break    *
 * conditions internally. _aqb will do this during signal handling  *
 * (i.e. SLEEP) automatically, for example.                         *
 *                                                                  *
 * The CHKBRK macro is provided for convenience.                    *
 *                                                                  *
 * __handle_break() will call the break_handler first if one is     *
 * registered via atbreak(). If this returns false or no            *
 * break_handler is registered, __handle_break() will either        *
 * generate a debugger TRAP if a debugger is connected or           *
 * _autil_exit() otherwise.                                         *
 *                                                                  *
 ********************************************************************/

#define BREAK_CTRL_C    1
#define BREAK_CTRL_D    2

extern USHORT _break_status;

#define CHKBRK if (_break_status) __handle_break()

void __handle_break(void);

/********************************************************************
 *                                                                  *
 *  memory management                                               *
 *                                                                  *
 ********************************************************************/

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

/********************************************************************
 *                                                                  *
 *  debug / trace utils                                             *
 *                                                                  *
 ********************************************************************/

void   _AQB_ASSERT   (BOOL b, const UBYTE *msg);

void _DEBUG_PUTC   (const char c);
void _DEBUG_PUTS   (const UBYTE *s);
void _DEBUG_PUTS1  (BYTE s);
void _DEBUG_PUTS2  (SHORT s);
void _DEBUG_PUTS4  (LONG l);
void _DEBUG_PUTU1  (UBYTE num);
void _DEBUG_PUTU2  (UWORD num);
void _DEBUG_PUTU4  (ULONG num);
void _DEBUG_PUTHEX (ULONG num);
void _DEBUG_PUTF   (FLOAT f);
void _DEBUG_PUTTAB (void);
void _DEBUG_PUTNL  (void);
void _DEBUG_CLS    (void);
void _DEBUG_BREAK  (void);

// #define ENABLE_DPRINTF

void dprintf(const char *format, ...);

#ifdef ENABLE_DPRINTF

#define DPRINTF(...) dprintf(__VA_ARGS__)

#else

#define DPRINTF(...)

#endif

/********************************************************************
 *                                                                  *
 *  BASIC error handling                                            *
 *                                                                  *
 ********************************************************************/

#define ERR_OUT_OF_DATA                4
#define ERR_ILLEGAL_FUNCTION_CALL      5
#define ERR_OUT_OF_MEMORY              7
#define ERR_SUBSCRIPT_OUT_OF_RANGE     9
#define ERR_INCOMPATIBLE_ARRAY        10

#define ERR_BAD_FILE_NUMBER           52
#define ERR_BAD_FILE_MODE             54
#define ERR_IO_ERROR                  57
#define ERR_BAD_FILE_NAME             64

void   ERROR         (SHORT errcode);
void   RESUME_NEXT   (void);
void   ON_ERROR_CALL (void (*cb)(void));
void   ON_BREAK_CALL (void (*cb)(void));

extern BOOL     _do_resume;     // set by RESUME NEXT

/********************************************************************
 *                                                                  *
 *  debugger connection                                             *
 *                                                                  *
 ********************************************************************/

#define DEBUG_SIG 0xDECA11ED

#define DEBUG_CMD_START    23
#define DEBUG_CMD_PUTC     24
#define DEBUG_CMD_PUTS     25

/* debugger sends this instead of WBStartup */
struct DebugMsg
{
    struct Message  msg;
    struct MsgPort *port;
    ULONG           debug_sig;                     // 24
    UWORD           debug_cmd;                     // 28
    ULONG           debug_exitFn;                  // 30
    union
    {
        ULONG   err;    // START return msg        // 34
        char    c;      // putc                    // 34
        char   *str;    // puts                    // 34
    }u;
};

extern struct DebugMsg *__StartupMsg;

//void *memset (void *dst, register int c, register int n);

//typedef long int SFVALUE;
#ifndef SItype
#define SItype long int
#endif

/*
 * time and date
 */

FLOAT  TIMER_        (void);
STRPTR DATE_         (void);
void   SLEEP_FOR     (FLOAT s);

/*
 * amath
 */

#define INTEGER_MIN -32768
#define INTEGER_MAX 32767

//FLOAT MOD_  (FLOAT divident, FLOAT divisor);

SHORT FIX_  (FLOAT f);
SHORT INT_  (FLOAT f);
SHORT CINT_ (FLOAT f);
LONG  CLNG_ (FLOAT a);

FLOAT __aqb_shl_single(FLOAT a, FLOAT b);
FLOAT __aqb_shr_single(FLOAT a, FLOAT b);

/*
 * string handling
 */

void _astr_itoa_ext       (LONG num, UBYTE *str, LONG base, BOOL leading_space, BOOL positive_sign);
void _astr_itoa           (LONG num, UBYTE *str, LONG base);
void _astr_utoa           (ULONG num, UBYTE* str, ULONG base);
void _astr_utoa_ext       (ULONG num, UBYTE* str, ULONG base, BOOL leading_space, BOOL positive_sign);
void _astr_ftoa           (FLOAT value, UBYTE *buf);
void _astr_ftoa_ext       (FLOAT value, UBYTE *buf, BOOL leading_space, BOOL positive_sign);

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

UBYTE *_S1TOA_   (BYTE   b);
UBYTE *_S2TOA_   (SHORT  i);
UBYTE *_S4TOA_   (LONG   l);
UBYTE *_U1TOA_   (UBYTE  b);
UBYTE *_U2TOA_   (USHORT i);
UBYTE *_U4TOA_   (ULONG  l);
UBYTE *_FTOA_    (FLOAT  f);
UBYTE *_BOOLTOA_ (BOOL   b);

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

/********************************************************************
 *                                                                  *
 *  OOP, Garbage Collector Interface (gc)                           *
 *                                                                  *
 ********************************************************************/

typedef struct CObject_ CObject;
typedef struct _gc_s    _gc_t;

struct CObject_
{
    VOID    ***_vTablePtr;

    CObject   *__gc_next, *__gc_prev;
    UBYTE      __gc_color;
};

void     _COBJECT___gc_scan    (CObject *THIS, _gc_t *gc);
STRPTR   _COBJECT_TOSTRING_    (CObject *THIS);
BOOL     _COBJECT_EQUALS_      (CObject *THIS, CObject *obj);
ULONG    _COBJECT_GETHASHCODE_ (CObject *THIS);

void     GC_RUN                (void);
void     GC_REGISTER           (CObject *obj);
CObject *GC_ALLOCATE_          (ULONG size, ULONG flags);
void     GC_MARK_BLACK         (CObject *obj);
BOOL     GC_REACHABLE_         (CObject *obj);

typedef void (*_gc_scan_t)     (CObject *obj);

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

VOID        _CARRAYENUMERATOR___init      (CArrayEnumerator *THIS);
VOID        _CARRAYENUMERATOR_CONSTRUCTOR (CArrayEnumerator *THIS, CArray *list);
BOOL        _CARRAYENUMERATOR_MOVENEXT_   (CArrayEnumerator *THIS);
intptr_t    _CARRAYENUMERATOR_CURRENT_    (CArrayEnumerator *THIS);
VOID        _CARRAYENUMERATOR_RESET       (CArrayEnumerator *THIS);

void        _CARRAY___init                (CArray *THIS);
VOID        _CARRAY_CONSTRUCTOR           (CArray *THIS, LONG     elementSize);
VOID        _CARRAY_REDIM                 (CArray *THIS, UWORD numDims, BOOL preserve, ...);
intptr_t   *_CARRAY_IDXPTR_               (CArray *THIS, UWORD    dimCnt, ...);
LONG        _CARRAY_LBOUND_               (CArray *THIS, WORD     d);
LONG        _CARRAY_UBOUND_               (CArray *THIS, WORD     d);
VOID        _CARRAY_COPY                  (CArray *THIS, CArray *darray);
VOID        _CARRAY_ERASE                 (CArray *THIS);
LONG        _CARRAY_COUNT_                (CArray *THIS);
VOID        _CARRAY_CAPACITY              (CArray *THIS, LONG     c);
LONG        _CARRAY_CAPACITY_             (CArray *THIS);
intptr_t    _CARRAY_GETAT_                (CArray *THIS, LONG     index);
VOID        _CARRAY_SETAT                 (CArray *THIS, LONG     index, intptr_t obj);
intptr_t ***_CARRAY_GETENUMERATOR_        (CArray *THIS);
LONG        _CARRAY_ADD_                  (CArray *THIS, intptr_t obj);
BOOL        _CARRAY_CONTAINS_             (CArray *THIS, intptr_t value);
VOID        _CARRAY_CLEAR                 (CArray *THIS);
BOOL        _CARRAY_ISREADONLY_           (CArray *THIS);
BOOL        _CARRAY_ISFIXEDSIZE_          (CArray *THIS);
LONG        _CARRAY_INDEXOF_              (CArray *THIS, intptr_t value, LONG     startIndex, LONG     Count);
VOID        _CARRAY_INSERT                (CArray *THIS, LONG     index, intptr_t value);
VOID        _CARRAY_REMOVE                (CArray *THIS, intptr_t value);
VOID        _CARRAY_REMOVEAT              (CArray *THIS, LONG     index);
VOID        _CARRAY_REMOVEALL             (CArray *THIS);
CObject    *_CARRAY_CLONE_                (CArray *THIS);

// DATA / READ / RESTORE support

void _AQB_RESTORE (void *p);
void _AQB_READ1   (void *v);
void _AQB_READ2   (void *v);
void _AQB_READ4   (void *v);
void _AQB_READSTR (void *v);

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

void _AIO_PUTS4                  (USHORT fno, LONG   num);
void _AIO_PUTS2                  (USHORT fno, SHORT  num);
void _AIO_PUTS1                  (USHORT fno, BYTE   num);
void _AIO_PUTU4                  (USHORT fno, ULONG  num);
void _AIO_PUTU2                  (USHORT fno, USHORT num);
void _AIO_PUTU1                  (USHORT fno, UBYTE  num);
void _AIO_PUTHEX                 (USHORT fno, LONG   num);
void _AIO_PUTUHEX                (USHORT fno, ULONG  l);
void _AIO_PUTBIN                 (USHORT fno, LONG   num);
void _AIO_PUTF                   (USHORT fno, FLOAT  f);
void _AIO_PUTBOOL                (USHORT fno, BOOL  b);

void _AIO_PUTS                   (USHORT fno, const UBYTE *str);

void _AIO_PUTNL                  (USHORT fno);
void _AIO_PUTTAB                 (USHORT fno);

struct FileHandle *_aio_getfh    (USHORT fno);

void _AIO_WRITES4                (USHORT fno, LONG   num);
void _AIO_WRITES2                (USHORT fno, SHORT  num);
void _AIO_WRITES1                (USHORT fno, BYTE   num);
void _AIO_WRITEU4                (USHORT fno, ULONG  num);
void _AIO_WRITEU2                (USHORT fno, USHORT num);
void _AIO_WRITEU1                (USHORT fno, UBYTE  num);
void _AIO_WRITEF                 (USHORT fno, FLOAT  f);
void _AIO_WRITEBOOL              (USHORT fno, BOOL   b);
void _AIO_WRITES                 (USHORT fno, const UBYTE *str);
void _AIO_WRITECOMMA             (USHORT fno, BOOL   b);

// [ LINE ] INPUT support:

void _AIO_LINE_INPUT             (USHORT fno, UBYTE *prompt, UBYTE **s, BOOL do_nl);
void _AIO_CONSOLE_INPUT          (BOOL qm, UBYTE *prompt, BOOL do_nl);
void _AIO_INPUTS1                (USHORT fno, BYTE   *v);
void _AIO_INPUTU1                (USHORT fno, UBYTE  *v);
void _AIO_INPUTS2                (USHORT fno, SHORT  *v);
void _AIO_INPUTU2                (USHORT fno, USHORT *v);
void _AIO_INPUTS4                (USHORT fno, LONG   *v);
void _AIO_INPUTU4                (USHORT fno, ULONG  *v);
void _AIO_INPUTF                 (USHORT fno, FLOAT  *v);
void _AIO_INPUTS                 (USHORT fno, UBYTE  **v);
void _AIO_SET_DOS_CURSOR_VISIBLE (BOOL visible);

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

void _AIO_OPEN  (UBYTE *fname, USHORT mode, USHORT access, USHORT fno, USHORT recordlen);
void _AIO_CLOSE (USHORT fno);

BOOL EOF_       (USHORT fno);

#endif
