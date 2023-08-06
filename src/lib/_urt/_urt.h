#ifndef HAVE_BRT_H
#define HAVE_BRT_H

/**********************************************************************
 **                                                                  **
 **  _urt: universal, minimalistic C#/VB.NET runtime                 **
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
 *              - call _acs_main()                                  *
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
 *  if _acs_stack_size in nonzero, a custom stack will be allocated *
 *  for the application. AllocVec() is done in _cstartup,           *
 *  Exec's StackSwap() is then called from startup.s                *
 *                                                                  *
 *  FIXME: check for current stack size, swap stack only if         *
 *         _acs_stack_size is larger then what we have already      *
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
extern ULONG         _acs_stack_size;
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
 * _urt installs an input handler (via input.device) which will     *
 * (just) the set _break_status global variable when a              *
 * CTRL-C/CTRL-D input event occurs.                                *
 *                                                                  *
 * the application needs to check for this condition at strategic   *
 * locations regularly and call __handle_break() or handle break    *
 * conditions internally. _acs will do this during signal handling  *
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
 *  OOP, Garbage Collector Interface (gc)                           *
 *                                                                  *
 ********************************************************************/

typedef struct CObject_ CObject;
typedef struct CString_ CString;
typedef struct _gc_s    _gc_t;

struct CObject_
{
    VOID    ***_vTablePtr;

    CObject   *__gc_next, *__gc_prev;
    ULONG      __gc_size;
    UBYTE      __gc_color;
};
extern ULONG *_td__urt_COBJECT;

void     _COBJECT___gc_scan    (CObject *THIS, _gc_t *gc);
CString *_COBJECT_TOSTRING_    (CObject *THIS);
BOOL     _COBJECT_EQUALS_      (CObject *THIS, CObject *obj);
ULONG    _COBJECT_GETHASHCODE_ (CObject *THIS);

BOOL     __instanceof          (CObject *obj, ULONG **td);

void     GC_RUN                (void);
void     GC_REGISTER           (CObject *obj);
CObject *GC_ALLOCATE_          (ULONG size, ULONG flags);
void     GC_MARK_BLACK         (CObject *obj);
BOOL     GC_REACHABLE_         (CObject *obj);
// FIXME: PUBLIC DECLARE EXTERN SUB      GC_OPTIONS    (BYVAL heap_limit AS ULONG=16*1024, BYVAL alloc_limit AS ULONG=128)

typedef void (*_gc_scan_t)     (CObject *obj);
typedef void (*_gc_finalize_t) (CObject *obj);

/*
 * internal string utils
 */

void _astr_itoa_ext       (LONG num, UBYTE *str, LONG base, BOOL leading_space, BOOL positive_sign);
void _astr_itoa           (LONG num, UBYTE *str, LONG base);
void _astr_utoa           (ULONG num, UBYTE* str, ULONG base);
void _astr_utoa_ext       (ULONG num, UBYTE* str, ULONG base, BOOL leading_space, BOOL positive_sign);
void _astr_ftoa           (FLOAT value, UBYTE *buf);
void _astr_ftoa_ext       (FLOAT value, UBYTE *buf, BOOL leading_space, BOOL positive_sign);

ULONG        _astr_len     (const UBYTE *str);
UBYTE       *_astr_dup     (const UBYTE *str);
SHORT        __astr_cmp    (const UBYTE *s1, const UBYTE *s2);
const UBYTE *_astr_strchr  (const UBYTE *s, UBYTE c);
UBYTE       *__astr_concat (const UBYTE *a, const UBYTE *b);

/*
 * STR$ support
 */

// FIXME UBYTE *_S1TOA_   (BYTE   b);
// FIXME UBYTE *_S2TOA_   (SHORT  i);
// FIXME UBYTE *_S4TOA_   (LONG   l);
// FIXME UBYTE *_U1TOA_   (UBYTE  b);
// FIXME UBYTE *_U2TOA_   (USHORT i);
// FIXME UBYTE *_U4TOA_   (ULONG  l);
// FIXME UBYTE *_FTOA_    (FLOAT  f);
// FIXME UBYTE *_BOOLTOA_ (BOOL   b);

// HEX$, OCT$, BIN$ support

// FIXME UBYTE *HEX_   (ULONG   l);
// FIXME UBYTE *OCT_   (ULONG   l);
// FIXME UBYTE *BIN_   (ULONG   l);

/*
 * VAL* support
 */

// FIXME LONG _str2i4_ (UBYTE *str, LONG len, LONG base);
// FIXME FLOAT _str2f_ (UBYTE *str, LONG len, LONG base);

// FIXME FLOAT  VAL_     (UBYTE *s);
// FIXME SHORT  VALINT_  (UBYTE *s);
// FIXME USHORT VALUINT_ (UBYTE *s);
// FIXME LONG   VALLNG_  (UBYTE *s);
// FIXME ULONG  VALULNG_ (UBYTE *s);

/*
 * BASIC string functions
 */

// FIXME ULONG  LEN_               (const UBYTE *str);
// FIXME UBYTE *SPACE_(SHORT length);
// FIXME UBYTE *SPC_(SHORT length);
// FIXME UBYTE *STRING_(SHORT length, UBYTE *str);
// FIXME UBYTE  *CHR_              (LONG codepoint);
// FIXME SHORT  ASC_               (const UBYTE *str);
// FIXME UBYTE  *MID_              (const UBYTE *str, SHORT n, SHORT m);
// FIXME UBYTE *UCASE_             (const UBYTE *s);
// FIXME UBYTE *LCASE_             (const UBYTE *s);
// FIXME UBYTE *LEFT_              (const UBYTE *s, SHORT n);
// FIXME UBYTE *RIGHT_             (const UBYTE *s, SHORT n);
// FIXME SHORT  INSTR              (SHORT n, const UBYTE *x, const UBYTE *y);


/********************************************************************
 *                                                                  *
 *  dynamic memory                                                  *
 *                                                                  *
 ********************************************************************/

APTR   ALLOCATE_   (ULONG size, ULONG flags);
void   DEALLOCATE  (APTR ptr);
void   _MEMSET     (BYTE *dst, BYTE c, ULONG n);
ULONG  FRE_        (SHORT x);

//void   POKE        (ULONG adr, UBYTE  b);
//void   POKEW       (ULONG adr, USHORT w);
//void   POKEL       (ULONG adr, ULONG  l);
//
//UBYTE  PEEK_       (ULONG adr);
//USHORT PEEKW_      (ULONG adr);
//ULONG  PEEKL_      (ULONG adr);
//
//ULONG  CRC32_      (const UBYTE *p, ULONG len);

/********************************************************************
 *                                                                  *
 *  debug / trace utils                                             *
 *                                                                  *
 ********************************************************************/

void   _ACS_ASSERT   (BOOL b, const UBYTE *msg);

void __debug_puts      (CONST_STRPTR s);
void __debug_put_int8  (int8_t c);
//void _DEBUG_PUTS1  (BYTE s);
void __debug_put_int16 (int16_t s);
//void _DEBUG_PUTS4  (LONG l);
//void _DEBUG_PUTU1  (UBYTE num);
//void _DEBUG_PUTU2  (UWORD num);
//void _DEBUG_PUTU4  (ULONG num);
//void _DEBUG_PUTHEX (ULONG num);
//void _DEBUG_PUTF   (FLOAT f);
//void _DEBUG_PUTTAB (void);
//void _DEBUG_PUTNL  (void);
//void _DEBUG_CLS    (void);
//void _DEBUG_BREAK  (void);

// #define ENABLE_DPRINTF

void dprintf(const char *format, ...);

#ifdef ENABLE_DPRINTF

#define DPRINTF(...) dprintf(__VA_ARGS__)

#else

#define DPRINTF(...)

#endif

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
 * amath
 */

#define INTEGER_MIN -32768
#define INTEGER_MAX 32767

//FLOAT MOD_  (FLOAT divident, FLOAT divisor);

//SHORT FIX_  (FLOAT f);
//SHORT INT_  (FLOAT f);
//SHORT CINT_ (FLOAT f);
//LONG  CLNG_ (FLOAT a);
//
//FLOAT __acs_shl_single(FLOAT a, FLOAT b);
//FLOAT __acs_shr_single(FLOAT a, FLOAT b);

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


#endif
