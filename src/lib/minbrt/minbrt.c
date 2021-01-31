/*
 * C part of minmal AQB runtime (debug purposes only)
 *
 * opens libraries, initializes other modules,
 * calls __aqbmain
 * and shuts down everything once __aqbmain returns
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <exec/execbase.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

// #define ENABLE_DEBUG
#define MAXBUF 40

#define ERR_OUT_OF_DATA              4
#define ERR_OUT_OF_MEMORY            7
#define ERR_SUBSCRIPT_OUT_OF_RANGE   9
#define ERR_INCOMPATIBLE_ARRAY      10

void _autil_exit(LONG return_code);
void _aqb_main(void);

struct ExecBase      *SysBase       = NULL;
struct DOSBase       *DOSBase       = NULL;
struct MathBase      *MathBase      = NULL;
struct MathTransBase *MathTransBase = NULL;

static FLOAT g_one_half, g_zero;

//static BOOL autil_init_done = FALSE;

/************************************************************************
 *
 * math
 *
 ************************************************************************/

/*
 * __pow_s4 / __pow_s2
 *
 * source: stackoverflow
 */

int32_t __pow_s4(int32_t base, int32_t exp)
{
    int32_t result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

uint32_t __pow_u4(uint32_t base, uint32_t exp)
{
    uint32_t result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

int16_t __pow_s2(int16_t base, int16_t exp)
{
    int16_t result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

uint16_t __pow_u2(uint16_t base, uint16_t exp)
{
    uint16_t result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

int8_t __pow_s1(int8_t base, int8_t exp)
{
    int8_t result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

int8_t __mul_s1(int8_t a, int8_t b)
{
    return a*b;
}

int8_t __div_s1(int8_t a, int8_t b)
{
    return a/b;
}

int8_t __mod_s1(int8_t a, int8_t b)
{
    return a%b;
}

uint8_t __pow_u1(uint8_t base, uint8_t exp)
{
    uint8_t result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

uint8_t __mul_u1(uint8_t a, uint8_t b)
{
    return a*b;
}

uint8_t __div_u1(uint8_t a, uint8_t b)
{
    return a/b;
}

uint8_t __mod_u1(uint8_t a, uint8_t b)
{
    return a%b;
}

SHORT fix_(FLOAT f)
{
    return SPFix(f);
}

SHORT int_(FLOAT f)
{
	if (SPCmp(f, g_zero)<0)
		return SPFix(SPSub(g_one_half, f));
    return SPFix(SPAdd(f, g_one_half));
}

SHORT cint_(FLOAT f)
{
	if (SPCmp(f, g_zero)<0)
		return SPFix(SPSub(g_one_half, f));
    return SPFix(SPAdd(f, g_one_half));
}

LONG clng_(FLOAT f)
{
	if (SPCmp(f, g_zero)<0)
		return SPFix(SPSub(g_one_half, f));
    return SPFix(SPAdd(f, g_one_half));
}

FLOAT __aqb_intdiv_single(FLOAT af, FLOAT bf)
{
    LONG a = clng_(af);
    LONG b = clng_(bf);

    return SPFlt(a / b);
}

FLOAT __aqb_shl_single(FLOAT af, FLOAT bf)
{
    LONG a = clng_(af);
    LONG b = clng_(bf);

    return SPFlt(a << b);
}

FLOAT __aqb_shr_single(FLOAT af, FLOAT bf)
{
    LONG a = clng_(af);
    LONG b = clng_(bf);

    return SPFlt(a >> b);
}

FLOAT __aqb_xor_single(FLOAT af, FLOAT bf)
{
    LONG a = clng_(af);
    LONG b = clng_(bf);

    return SPFlt(a ^ b);
}

FLOAT __aqb_eqv_single(FLOAT af, FLOAT bf)
{
    LONG a = clng_(af);
    LONG b = clng_(bf);

    return SPFlt(~(a ^ b));
}

FLOAT __aqb_imp_single(FLOAT af, FLOAT bf)
{
    LONG a = clng_(af);
    LONG b = clng_(bf);

    return SPFlt(~a | b);
}

FLOAT __aqb_not_single(FLOAT af)
{
    LONG a = clng_(af);

    return SPFlt(~a);
}

FLOAT __aqb_and_single(FLOAT af, FLOAT bf)
{
    LONG a = clng_(af);
    LONG b = clng_(bf);

    return SPFlt(a & b);
}

FLOAT __aqb_or_single(FLOAT af, FLOAT bf)
{
    LONG a = clng_(af);
    LONG b = clng_(bf);

    return SPFlt(a | b);
}

/************************************************************************
 *
 * i/o
 *
 ************************************************************************/

static BPTR _debug_stdout = 0;

ULONG len_(const char *str)
{
    int l = 0;
    while (*str)
    {
        str++;
        l++;
    }
    return l;
}

#if 1
/* A utility function to reverse a string  */
static void reverse(char *str, int length)
{
    int start = 0;
    int end   = length -1;
    while (start < end)
    {
        char c = *(str+start);
        *(str+start) = *(str+end);
        *(str+end)   = c;
        start++;
        end--;
    }
}

void _astr_itoa_ext(int num, char* str, int base, BOOL leading_space)
{
    int i = 0;
    BOOL isNegative = FALSE;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        if (leading_space)
            str[i++] = ' ';
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // In standard itoa(), negative numbers are handled only with
    // base 10. Otherwise numbers are considered unsigned.
    if (num < 0 && base == 10)
    {
        isNegative = TRUE;
        num = -num;
    }

    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }

    // If number is negative, append '-', else ' '
    if (isNegative)
    {
        str[i++] = '-';
    }
    else
    {
        if (leading_space)
            str[i++] = ' ';
    }

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    reverse(str, i);
}

void _astr_itoa(int num, char* str, int base)
{
    _astr_itoa_ext(num, str, base, /*leading_space=*/TRUE);
}
#else
void _astr_itoa(int num, char* str)
{
    str[0] = '1';
    str[1] = '2';
    str[2] = 0;
}
#endif

void _debug_puts(const char *s)
{
    if (_debug_stdout)
        Write(_debug_stdout, (CONST APTR) s, len_(s));
}

void _debug_puts2(SHORT s)
{
    char buf[MAXBUF];
    _astr_itoa(s, buf, 10);
    _debug_puts(buf);
}

void _debug_putu4(ULONG l)
{
    char buf[MAXBUF];
    _astr_itoa(l, buf, 10);
    _debug_puts(buf);
}

void _debug_putnl(void)
{
    if (_debug_stdout)
        Write(_debug_stdout, "\n", 1);
}

void _debug_cls(void)
{
    if (_debug_stdout)
        Write(_debug_stdout, "\f", 1);
}

/************************************************************************
 *
 * ALLOCATE / DEALLOCATE
 *
 ************************************************************************/

// not using Intuition's AllocRemember here because we want minimal dependencies for the AQB core module

typedef struct AQB_memrec_ *AQB_memrec;

struct AQB_memrec_
{
    AQB_memrec next;
    ULONG      size;
    APTR      *mem;
};

static AQB_memrec g_mem = NULL;

APTR ALLOCATE_(ULONG size, ULONG flags)
{
    AQB_memrec mem_prev = g_mem;

    //_debug_puts("ALLOCATE size=");
    //_debug_puts2(size);
    //_debug_puts(", flags=");
    //_debug_puts2(flags);
    //_debug_puts("\n");

    g_mem = (AQB_memrec) AllocMem (sizeof(*g_mem), 0);
    if (!g_mem)
    {
        g_mem = mem_prev;
        return NULL;
    }

    g_mem->mem = (APTR) AllocMem (size, flags);
    if (!g_mem->mem)
    {
        FreeMem(g_mem, sizeof (*g_mem));
        g_mem = mem_prev;
        return NULL;
    }

    g_mem->size = size;
    g_mem->next = mem_prev;

    return g_mem->mem;
}

void DEALLOCATE (APTR ptr, ULONG size)
{
    // FIXME: implement.
}

/************************************************************************
 *
 * string support
 *
 ************************************************************************/

char *_astr_dup(const char* str)
{
    ULONG l = len_(str);
    char *str2 = ALLOCATE_(l+1, MEMF_ANY);
    CopyMem((APTR)str, (APTR)str2, l+1);
    return str2;
}

/************************************************************************
 *
 * ON ERROR ...
 *
 ************************************************************************/

static void (*error_handler)(void) = NULL;
static BOOL do_resume = FALSE;

SHORT ERR=0;

void ON_ERROR_CALL(void (*cb)(void))
{
    error_handler = cb;
}

void ERROR (SHORT errcode)
{
    do_resume = FALSE;
    ERR = errcode;

    if (error_handler)
    {
        error_handler();
    }
    else
    {
        _debug_puts("*** unhandled runtime error code: "); _debug_puts2(errcode);
        _debug_puts("\n");
    }

    if (!do_resume)
        _autil_exit(errcode);
    else
        ERR=0;
}

void RESUME_NEXT(void)
{
    do_resume = TRUE;
}

/************************************************************************
 *
 * DATA / READ / RESTORE support
 *
 ************************************************************************/

static void *g_data_ptr=NULL;

void _aqb_restore (void *p)
{
    g_data_ptr = p;
}

void _aqb_read1 (void *v)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    *((BYTE*) v) = *((BYTE *)g_data_ptr);

    g_data_ptr += 1;
}

void _aqb_read2 (void *v)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    *((SHORT*) v) = *((SHORT *)g_data_ptr);

    g_data_ptr += 2;
}

void _aqb_read4 (void *v)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    *((LONG*) v) = *((LONG *)g_data_ptr);

    g_data_ptr += 4;
}

#define MAX_STRING_LEN 1024

void _aqb_readStr (void *v)
{
    char buf[MAX_STRING_LEN];
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    char c = 0xff;
    int l = 0;
    while (c && (l<MAX_STRING_LEN-1))
    {
        c = buf[l] = *((char *)g_data_ptr);
        // _debug_puts("_aqb_readStr: c="); _debug_puts2(c); _debug_putnl();
        g_data_ptr += 1;
        l++;
    }
    buf[l] = 0;
    *((char **)v) = _astr_dup(buf);
}

/************************************************************************
 *
 * darray support
 *
 ************************************************************************/

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

void __DARRAY_T___init__ (_DARRAY_T *self, ULONG elementSize)
{
    // _debug_puts ("__DARRAY_T___init__: elementSize="); _debug_puts2(elementSize); _debug_putnl();
    self->data        = NULL;
    self->numDims     = 0;
    self->elementSize = elementSize;
    self->dataSize    = 0;
    self->bounds      = NULL;
}

void __DARRAY_T_REDIM (_DARRAY_T *self, BOOL preserve, UWORD numDims, ...)
{
    va_list valist;

    self->numDims     = numDims;

    self->bounds = ALLOCATE_ (sizeof (_DARRAY_BOUNDS_T) * numDims, 0);

    va_start (valist, numDims);
    ULONG dataSize = self->elementSize;
    for (UWORD iDim=0; iDim<numDims; iDim++)
    {
        ULONG start = va_arg(valist, ULONG);
        ULONG end   = va_arg(valist, ULONG);
        dataSize *= end - start + 1;
        //_debug_puts ("_dyna_redim: dim: start="); _debug_puts2(start); _debug_puts(", end="); _debug_puts2(end); _debug_putnl();
        self->bounds[iDim].lbound      = start;
        self->bounds[iDim].ubound      = end;
        self->bounds[iDim].numElements = end-start+1;
    }
    va_end(valist);

    APTR oData      = self->data;
    ULONG oDataSize = self->dataSize;

    self->data     = ALLOCATE_ (dataSize, 0);
    self->dataSize = dataSize;

    if (oData)
    {
        if (preserve)
        {
            ULONG toCopy = dataSize < oDataSize ? dataSize : oDataSize;
            CopyMem (oData, self->data, toCopy);
            //_debug_puts ("_dyna_redim: preserve, toCopy="); _debug_puts2(toCopy); _debug_putnl();
        }
        DEALLOCATE (oData, oDataSize);
    }

    //_debug_puts ("_dyna_redim: self="); _debug_putu4((ULONG) self); _debug_puts(", data="); _debug_putu4((ULONG)self->data); _debug_puts (", dataSize="); _debug_puts2(dataSize); _debug_puts (", numDims="); _debug_puts2(numDims); _debug_putnl();
}

/*
 * elements are in row-major order
 * array dimensions: a(N0, N1, N2, ...), lement size E
 * e.g. DIM a(4, 3), E=4          / e_0_0[ 0]  e_0_1[ 4]  e_0_2[ 8] \
 *                               /  e_1_0[12]  e_1_1[16]  e_1_2[20]  \
 *                               \  e_2_0[24]  e_2_1[28]  e_2_2[32]  /
 *                                \ e_3_0[36]  e_3_1[40]  e_3_2[44] /
 *
 * offset calc for a(i0, i1)        , element size E : o = E*i1 + E*N1*i0
 *                 a(i0, i1, i2, i3)                   o = E*i3 + E*N3*i2 + E*N2*N3*i1 + E*N1*N2*N3*i0
 *
 *
 *
 */

void *__DARRAY_T_IDXPTR_ (_DARRAY_T *self, UWORD dimCnt, ...)
{
    //int i = foobar(dimCnt);

    //_dyna_create_ (dimCnt, i, 1, 2);

    //_debug_puts ("_dyna_idx: dimCnt="); _debug_puts2(dimCnt); _debug_putnl();

    if (!self->data)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    if (dimCnt != self->numDims)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    va_list valist;
    va_start (valist, dimCnt);
    ULONG offset = 0;
    ULONG es     = self->elementSize;
    for (WORD iDim=self->numDims-1; iDim>=0; iDim--)
    {
        ULONG lbound = self->bounds[iDim].lbound;
        ULONG ubound = self->bounds[iDim].ubound;
        ULONG n     = self->bounds[iDim].numElements;

        //_debug_puts ("_dyna_idx: dim: iDim="); _debug_puts2(iDim); _debug_puts(", lbound="); _debug_puts2(lbound); _debug_putnl();

        ULONG idx = va_arg(valist, ULONG);

        if ((idx<lbound) || (idx>ubound))
            ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

        offset += es * (idx - lbound);
        es *= n;
        //_debug_puts ("_dyna_idx: dim: idx="); _debug_puts2(idx); _debug_puts(", offset="); _debug_puts2(offset); _debug_putnl();
    }
    va_end(valist);

    void *ptr = self->data+offset;
    //_debug_puts ("_dyna_idx: self="); _debug_putu4((ULONG) self); _debug_puts(", data="); _debug_putu4((ULONG)self->data); _debug_puts (" -> ptr="); _debug_putu4((ULONG)ptr); _debug_putnl();

    return ptr;
}

WORD  __DARRAY_T_LBOUND_  (_DARRAY_T *self, WORD d)
{
    if (d<=0)
        return 1;

    if (!self->data)
        return 0;

    if (d>self->numDims)
        return 0;

    return self->bounds[d-1].lbound;
}

WORD  __DARRAY_T_UBOUND_  (_DARRAY_T *self, WORD d)
{
    if (d<=0)
        return self->numDims;

    if (!self->data)
        return -1;

    if (d>self->numDims)
        return 0;

    return self->bounds[d-1].ubound;
}

void __DARRAY_T_COPY (_DARRAY_T *self, _DARRAY_T *a)
{
    if (a->numDims != self->numDims)
        ERROR (ERR_INCOMPATIBLE_ARRAY);

    ULONG toCopy = a->dataSize < self->dataSize ? a->dataSize : self->dataSize;

    //_debug_puts ("__DARRAY_T_COPY: toCopy="); _debug_puts2(toCopy); _debug_putnl();
    CopyMem(a->data, self->data, toCopy);
}

void __DARRAY_T_ERASE (_DARRAY_T *self)
{
    if (self->data)
    {
        DEALLOCATE (self->data, self->dataSize);
    }
    // FIXME: free bounds!

    self->data        = NULL;
    self->numDims     = 0;
    self->dataSize    = 0;
    self->bounds      = NULL;
}



/************************************************************************
 *
 * startup / exit
 *
 ************************************************************************/

void _aqb_assert (BOOL b, const char *msg)
{
    if (b)
        return;

    _debug_puts(msg);
    _debug_puts("\n");

    _autil_exit(20);
}

void SYSTEM(void)
{
    _autil_exit(0);
}

// gets called by _autil_exit
void _minbrt_exit(void)
{

    while (g_mem)
    {
        AQB_memrec mem_next = g_mem->next;
        FreeMem(g_mem->mem, g_mem->size);
        FreeMem(g_mem, sizeof (*g_mem));
        g_mem = mem_next;
    }

#ifdef ENABLE_DEBUG
    if (DOSBase)
        _debug_puts("_c_atexit...\n");
#endif

#ifdef ENABLE_DEBUG
    if (DOSBase)
        _debug_puts("_c_atexit... finishing.\n");
#endif

    if (DOSBase)
        CloseLibrary( (struct Library *)DOSBase);
}

void _cshutdown (LONG return_code, char *msg)
{
    if (msg && DOSBase)
        _debug_puts(msg);

    _autil_exit(return_code);
}

// called from aqb main, unused here
void __brt_init(void)
{
}

static void _minbrt_init (void)
{
    SysBase = (*((struct ExecBase **) 4));

    if (!(DOSBase = (struct DOSBase *)OpenLibrary((CONST_STRPTR) "dos.library", 0)))
        _cshutdown(20, "*** error: failed to open dos.library!\n");

    _debug_stdout = Output();

    if (!(MathBase = (struct MathBase *)OpenLibrary((CONST_STRPTR) "mathffp.library", 0)))
        _cshutdown(20, "*** error: failed to open mathffp.library!\n");

    if (!(MathTransBase = (struct MathTransBase *)OpenLibrary((CONST_STRPTR) "mathtrans.library", 0)))
        _cshutdown(20, "*** error: failed to open mathtrans.library!\n");

    g_one_half = SPDiv(SPFlt(2), SPFlt(1));
    g_zero     = SPFlt(0);
}


void _minbrt_startup (void)
{
    _minbrt_init();

    _aqb_main();

    _autil_exit(0);
}


