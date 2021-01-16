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


