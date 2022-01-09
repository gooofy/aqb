#include "_brt.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#include <clib/mathtrans_protos.h>
#include <inline/mathtrans.h>

#include <clib/utility_protos.h>
#include <inline/utility.h>

#define MAXBUF 40

//#define DEBUG

extern struct UtilityBase   *UtilityBase;

/* A utility function to reverse a string  */
static void reverse(UBYTE *str, LONG length)
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

void _astr_itoa_ext(LONG num, UBYTE* str, LONG base, BOOL leading_space)
{
    LONG i = 0;
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

void _astr_itoa(LONG num, UBYTE* str, LONG base)
{
    _astr_itoa_ext(num, str, base, /*leading_space=*/TRUE);
}

void _astr_utoa(ULONG num, UBYTE* str, ULONG base)
{
    int i = 0;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
        str[i++] = ' ';
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Process individual digits
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }

    str[i++] = ' ';  // Append space (not negative)
    str[i] = '\0'; // Append string terminator

    // Reverse the string
    reverse(str, i);
}

ULONG LEN_(const UBYTE *str)
{
    int l = 0;
    while (*str)
    {
        str++;
        l++;
    }
    return l;
}

UBYTE *CHR_(LONG codepoint)
{
    UBYTE s[2];
    s[0] = codepoint;
    s[1] = 0;
    return _astr_dup(s);
}

SHORT ASC_(const UBYTE *str)
{
    if (!str)
    {
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
        return 0;
    }

    int l = LEN_(str);
    if (!l)
    {
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
        return 0;
    }

    return str[0];
}

UBYTE *MID_(const UBYTE *str, SHORT n, SHORT m)
{
    DPRINTF ("mid$: str=0x%08lx, n=%d, m=%d\n", str, n, m);
    if (!str || (n<1))
    {
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
        return _astr_dup((STRPTR)"");
    }
    int l = LEN_(str);
    n--;
    if (n>=l)
        return _astr_dup((STRPTR)"");
    int l2 = l-n;
    if ((m>=0) && (m<l2))
        l2 = m;

    UBYTE *str2 = ALLOCATE_(l2+1, MEMF_ANY);
    CopyMem((APTR) (str+n), (APTR)str2, l2);
    str2[l2]=0;
    return str2;
}

UBYTE *UCASE_ (const UBYTE *s)
{
    if (!s)
    {
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
        return _astr_dup((STRPTR)"");
    }
    UBYTE *s2 = _astr_dup(s);
    int l = LEN_(s2);
    for (int i=0; i<l; i++)
        s2[i] =ToUpper(s[i]);

    return s2;
}

UBYTE *LCASE_ (const UBYTE *s)
{
    if (!s)
    {
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
        return _astr_dup((STRPTR)"");
    }
    UBYTE *s2 = _astr_dup(s);
    int l = LEN_(s2);
    for (int i=0; i<l; i++)
        s2[i] =ToLower(s[i]);

    return s2;
}

UBYTE *LEFT_ (const UBYTE *s, SHORT n)
{
    if (!s || (n<0))
    {
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
        return _astr_dup((STRPTR)"");
    }

    int l = LEN_(s);
    if (n>l)
        n = l;

    UBYTE *str2 = ALLOCATE_(n+1, MEMF_ANY);
    CopyMem((APTR) (s), (APTR)str2, n);
    str2[n]=0;

    return str2;
}

UBYTE *RIGHT_ (const UBYTE *s, SHORT n)
{
    if (!s || (n<0))
    {
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
        return _astr_dup((STRPTR)"");
    }
    int l = LEN_(s);
    if (n>l)
        n = l;

    return MID_(s, l-n+1, -1);
}

const UBYTE *_astr_strstr(const UBYTE *s1, const UBYTE *s2)
{

	const UBYTE *c1, *c2;

    do
	{
        c1 = s1; c2 = s2;
        while (*c1 && *c1==*c2)
		{
            c1++;
			c2++;
        }

        if (!*c2)
            return s1;

    } while (*s1++);

    return NULL;
}


SHORT INSTR_ (SHORT n, const UBYTE *x, const UBYTE *y)
{
    n-=1;
    if (!x || !y || (n<0))
    {
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
        return 0;
    }

    int l = LEN_(x);
    if (n>=l)
	{
        return 0;
	}

	x += n;

	const UBYTE *s = _astr_strstr(x, y);
	if (!s)
		return 0;

	return s-x+1+n;
}

UBYTE *_astr_dup(const UBYTE* str)
{
    ULONG l = LEN_(str);
    UBYTE *str2 = ALLOCATE_(l+1, MEMF_ANY);
    CopyMem((APTR)str, (APTR)str2, l+1);
    return str2;
}

SHORT __astr_cmp(const UBYTE* s1, const UBYTE* s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const UBYTE*)s1 - *(const UBYTE*)s2;
}

UBYTE *__astr_concat (const UBYTE *a, const UBYTE *b)
{
    ULONG la = LEN_(a);
    ULONG lb = LEN_(b);
    UBYTE *str2 = ALLOCATE_(la+lb+1, MEMF_ANY);
    CopyMem((APTR)a, (APTR)str2, la);
    CopyMem((APTR)b, (APTR)str2+la, lb+1);
    return str2;
}

/*
 * Lightweight float to string conversion
 *
 * source: https://blog.benoitblanchon.fr/lightweight-float-to-string/
 * author: Benoit Blanchon
 */

/* Motorola FFP format
 _____________________________________________
|                                             |
| MMMMMMMM    MMMMMMMM    MMMMMMMM    EEEEEEE |
| 31          23          15          7       |
|_____________________________________________|
*/

static FLOAT g_positiveExpThreshold;
static FLOAT g_negativeExpThreshold;
static FLOAT g_1e16, g_1e8, g_1e4, g_1e2, g_1e1, g_1e9, g_0, g_1, g_10;
static FLOAT g_1en15, g_1en7, g_1en3, g_1en1, g_05, g_m1;

// normalizes the value between 1e-5 and 1e7 and returns the exponent
static SHORT normalizeFloat(FLOAT *value)
{

    SHORT exponent = 0;

    if (SPCmp(*value, g_positiveExpThreshold) >=0)
    {
        if (SPCmp(g_1e16, *value) >= 0)
        {
            *value = SPDiv(g_1e16, *value);
            exponent += 16;
        }
        if (SPCmp(g_1e8, *value) >= 0)
        {
            *value = SPDiv(g_1e8, *value);
            exponent += 8;
        }
        if (SPCmp(g_1e4, *value) >= 0)
        {
            *value = SPDiv(g_1e4, *value);
            exponent += 4;
        }
        if (SPCmp(g_1e2, *value) >= 0)
        {
            *value = SPDiv(g_1e2, *value);
            exponent += 2;
        }
        if (SPCmp(g_1e1, *value) >= 0)
        {
            *value = SPDiv(g_1e1, *value);
            exponent += 1;
        }
    }

    if ( (SPCmp(0, *value) > 0) &&
         (SPCmp(g_negativeExpThreshold, *value) <= 0) )
    {
        if (SPCmp(g_1en15, *value) <0)
        {
            *value = SPMul(*value, g_1e16);
            exponent -= 16;
        }
        if (SPCmp(g_1en7, *value) <0)
        {
            *value  = SPMul(*value, g_1e8);
            exponent -= 8;
        }
        if (SPCmp(g_1en3, *value) <0)
        {
            *value  = SPMul(*value, g_1e4);
            exponent -= 4;
        }
        if (SPCmp(g_1en1, *value) <0)
        {
            *value  = SPMul(*value, g_1e2);
            exponent -= 2;
        }
        if (SPCmp(g_1, *value) <0)
        {
            *value  = SPMul(*value, g_1e1);
            exponent -= 1;
        }
    }

#ifdef DEBUG
    _aio_puts("exponent:"); _aio_puts4(exponent); _aio_putnl();
#endif

    return exponent;
}

void _astr_ftoa(FLOAT value, UBYTE *buf)
{
    BOOL negative = FALSE;

    // if (isnan(value)) return _aio_puts("nan");

    if (SPCmp(value, g_0)<0)
    {
        value = SPMul(value, g_m1);
        negative = TRUE;
#ifdef DEBUG
        _aio_puts("negative.\n");
#endif
    }

    // if (isinf(value)) return _aio_puts("inf");

    /*
     * split float into integral part, decimal part and exponent
     */

    LONG integralPart;
    ULONG decimalPart;
    SHORT exponent;

    exponent = normalizeFloat(&value);

    integralPart = SPFix(value);
    FLOAT remainder = SPSub(SPFlt(integralPart), value);
#ifdef DEBUG
    _aio_puts("integralPart:"); _aio_puts4(integralPart); _aio_putnl();
    _aio_puts("10-4="); _aio_puts4(SPFix(SPSub(SPFlt(4), SPFlt(10)))); _aio_putnl();
    _aio_puts("2^3=");  _aio_puts4(SPFix(SPPow(SPFlt(3), SPFlt(2)))); _aio_putnl();
    _aio_puts("12/3=");  _aio_puts4(SPFix(SPDiv(SPFlt(3), SPFlt(12)))); _aio_putnl();
#endif

    remainder = SPMul(remainder, g_1e9);
    decimalPart = SPFix(remainder);
#ifdef DEBUG
    _aio_puts("decimalPart:"); _aio_puts4(decimalPart); _aio_putnl();
#endif

    // rounding
    remainder = SPSub(SPFlt(decimalPart), remainder);
    if (SPCmp(g_05, remainder) >=0)
    {
        decimalPart = decimalPart + 1;
        if (decimalPart >= 1000000000)
        {
            decimalPart = 0;
            integralPart = integralPart + 1;
            if ((exponent != 0) && (integralPart >= 10))
            {
                exponent = exponent + 1;
                integralPart = 1;
            }
        }
    }

    /*
     * produce ascii string
     */


    _astr_itoa(integralPart, &buf[0], 10);
    if (negative)
        buf[0] = '-';

    if (decimalPart)
    {
        int width = 9;
        // writeDecimals(decimalPart, &buf[l]);

        // remove trailing zeros
        while (decimalPart % 10 == 0 && width > 0)
        {
            decimalPart /= 10;
            width--;
        }

        UBYTE buffer[16];
        UBYTE *ptr = buffer + sizeof(buffer) - 1;

        // write the string in reverse order
        *ptr = 0;
        while (width--)
        {
            *--ptr = decimalPart % 10 + '0';
            decimalPart /= 10;
        }
        *--ptr = '.';

        // and dump it in the right order
        ULONG l = LEN_(buf);
        CopyMem((APTR) ptr, (APTR) &buf[l], LEN_(ptr)+1);
    }

    if (exponent != 0)
    {
        ULONG l = LEN_(buf);
        buf[l] = 'e'; buf[l+1] = '-';
        _astr_itoa(exponent, &buf[l+1], 10);
    }
}

const UBYTE *_astr_strchr(const UBYTE *s, UBYTE c)
{
    const UBYTE *s2 = s;

    for (; *s2; s2++)
        if (*s2 == c)
            return s2;

    return NULL;
}

UBYTE *_s1toa_   (BYTE   b)
{
    UBYTE buf[MAXBUF];
    _astr_itoa(b, buf, 10);
    return _astr_dup(buf);
}

UBYTE *_s2toa_   (SHORT  i)
{
    UBYTE buf[MAXBUF];
    _astr_itoa(i, buf, 10);
    return _astr_dup(buf);
}

UBYTE *_s4toa_   (LONG   l)
{
    UBYTE buf[MAXBUF];
    _astr_itoa(l, buf, 10);
    return _astr_dup(buf);
}

UBYTE *_u1toa_   (UBYTE  b)
{
    UBYTE buf[MAXBUF];
    _astr_utoa(b, buf, 10);
    return _astr_dup(buf);
}

UBYTE *_u2toa_   (USHORT i)
{
    UBYTE buf[MAXBUF];
    _astr_utoa(i, buf, 10);
    return _astr_dup(buf);
}

UBYTE *_u4toa_   (ULONG  l)
{
    UBYTE buf[MAXBUF];
    _astr_utoa(l, buf, 10);
    return _astr_dup(buf);
}

UBYTE *_ftoa_    (FLOAT  f)
{
    UBYTE buf[MAXBUF];
    _astr_ftoa(f, buf);
    return _astr_dup(buf);
}

UBYTE *_booltoa_ (BOOL   b)
{
    return b ? (UBYTE *)"TRUE" : (UBYTE *)"FALSE";
}

/*
 * VAL* support
 */

LONG _str2i4_ (UBYTE *str, LONG len, LONG base)
{
    LONG v = 0;
    BOOL negative = FALSE;

    if (str[0]=='-')
    {
        negative = TRUE;
        str++;
    }
    else
    {
        if (str[0]=='+')
        {
            str++;
        }
    }

    int c;
    switch (base)
    {
        /* hex */
        case 16:
            while (--len >= 0)
            {
                c = *str++;
                if ((c >= 97) && (c <= 102))        // a-f
                    c -= 87;
                else if ((c >= 65) && (c <= 70))    // A-F
                    c -= 55;
                else if ((c >= 48) && (c <= 57))    // 0-9
                    c -= 48;
                else
                    break;
                v = (v * 16) + c;
            }
            break;

        /* dec */
        case 10:
            while (--len >= 0)
            {
                c = *str++;
                if ((c >= 48) && (c <= 57))         // 0-9
                    c -= 48;
                else
                    break;
                v = (v * 10) + c;
            }
            break;

        /* oct */
        case 8:
            while (--len >= 0)
            {
                c = *str++;
                if ((c >= 48) && (c <= 55))
                    v = (v * 8) + (c - 48);
                else
                    break;
            }
            break;

        /* bin */
        case 2:
            while (--len >= 0)
            {
                c = *str++;
                if ((c >= 48) && (c <= 49))
                    v = (v * 2) + (c - 48);
                else
                    break;
            }
            break;

        default:
            break;
    }

    if (negative)
        v = v * -1;

    return v;
}

static UBYTE *_val_handle_prefix(UBYTE *s, LONG *base, LONG *len)
{
    // skip whitespace

    UBYTE *p = s;
    while ( (*p == ' ') || (*p == '\t') )
        p++;

    *len = LEN_(p);
    if (!(*len))
        return s;

    *base = 10;
    int skip = 0;

    if (((*len) >= 2) && (p[0] == '&'))
    {
        skip = 2;
        switch(p[1])
        {
            case 'h':
            case 'H':
                *base = 16;
                break;

            case 'o':
            case 'O':
                *base = 8;
                break;

            case 'b':
            case 'B':
                *base = 2;
                break;

            default: /* assume octal */
                *base = 8;
                skip = 1;
                break;
        }
    }

    *len -= skip;
    return &p[skip];
}

SHORT VALINT_ (UBYTE *s)
{
    if (!s)
        return 0;

    LONG base=10, len=0;
    s = _val_handle_prefix(s, &base, &len);

    return _str2i4_(s, len, base);
}

USHORT VALUINT_ (UBYTE *s)
{
    if (!s)
        return 0;

    LONG base=10, len=0;
    s = _val_handle_prefix(s, &base, &len);

    LONG l = _str2i4_(s, len, base);
    return (USHORT) l;
}

LONG VALLNG_ (UBYTE *s)
{
    if (!s)
        return 0;

    LONG base=10, len=0;
    s = _val_handle_prefix(s, &base, &len);

    return _str2i4_(s, len, base);
}

ULONG VALULNG_ (UBYTE *s)
{
    if (!s)
        return 0;

    LONG base=10, len=0;
    s = _val_handle_prefix(s, &base, &len);

    return (ULONG) _str2i4_(s, len, base);
}

FLOAT _str2f_ (UBYTE *str, LONG len, LONG base)
{
    LONG v = 0;
    BOOL negative = FALSE;

    if (str[0]=='-')
    {
        negative = TRUE;
        str++;
    }
    else
    {
        if (str[0]=='+')
        {
            str++;
        }
    }

    LONG c;
    switch (base)
    {
        /* hex */
        case 16:
            while (len)
            {
                c = *str;
                if ((c >= 97) && (c <= 102))        // a-f
                    c -= 87;
                else if ((c >= 65) && (c <= 70))    // A-F
                    c -= 55;
                else if ((c >= 48) && (c <= 57))    // 0-9
                    c -= 48;
                else
                    break;
                v = (v * 16) + c;
                str++; len--;
            }
            break;

        /* dec */
        case 10:
            while (len)
            {
                c = *str;
                if ((c >= 48) && (c <= 57))         // 0-9
                    c -= 48;
                else
                    break;
                v = (v * 10) + c;
                str++; len--;
            }
            break;

        /* oct */
        case 8:
            while (len)
            {
                c = *str;
                if ((c >= 48) && (c <= 55))
                    v = (v * 8) + (c - 48);
                else
                    break;
                str++; len--;
            }
            break;

        /* bin */
        case 2:
            while (len)
            {
                c = *str;
                if ((c >= 48) && (c <= 49))
                    v = (v * 2) + (c - 48);
                else
                    break;
                str++; len--;
            }
            break;

        default:
            break;
    }

    // _debug_puts ("str2f: integer part v="); _debug_puts2(v); _debug_putnl();

    /*
     * fractional part
     */
    FLOAT f = SPFlt(v);
    FLOAT fBase = SPFlt(base);

    if (str[0]=='.')
    {
        str++;
        len--;
        FLOAT frac = SPDiv(fBase, g_1);

        switch (base)
        {
            /* hex */
            case 16:
                while (len)
                {
                    c = *str;
                    if ((c >= 97) && (c <= 102))        // a-f
                        c -= 87;
                    else if ((c >= 65) && (c <= 70))    // A-F
                        c -= 55;
                    else if ((c >= 48) && (c <= 57))    // 0-9
                        c -= 48;
                    else
                        break;
                    f = SPAdd(f, SPMul(SPFlt(c), frac));
                    frac = SPDiv(fBase, frac);
                    str++; len--;
                }
                break;

            /* dec */
            case 10:
                while (len)
                {
                    c = *str;
                    if ((c >= 48) && (c <= 57))         // 0-9
                        c -= 48;
                    else
                        break;
                    f = SPAdd(f, SPMul(SPFlt(c), frac));
                    frac = SPDiv(fBase, frac);
                    str++; len--;
                }
                break;

            /* oct */
            case 8:
                while (len)
                {
                    c = *str;
                    if ((c >= 48) && (c <= 55))
                        v = (v * 8) + (c - 48);
                    else
                        break;
                    f = SPAdd(f, SPMul(SPFlt(c), frac));
                    frac = SPDiv(fBase, frac);
                    str++; len--;
                }
                break;

            /* bin */
            case 2:
                while (len)
                {
                    c = *str;
                    if ((c >= 48) && (c <= 49))
                        v = (v * 2) + (c - 48);
                    else
                        break;
                    f = SPAdd(f, SPMul(SPFlt(c), frac));
                    frac = SPDiv(fBase, frac);
                    str++; len--;
                }
                break;

            default:
                break;
        }
    }

    //_debug_puts ("str2f: fractional part handled, f="); _debug_putf(f); _debug_putnl();

    if ( (str[0]=='e') || (str[0]=='E') )
    {
        str++; len--;
        BOOL expSign = FALSE;
        if (str[0] == '-')
        {
            expSign = TRUE;
            str++; len--;
        }
        else
        {
            if (str[0] == '+')
            {
                str++; len--;
            }
            expSign = FALSE;
        }
        FLOAT exp = g_0;
        while (--len >= 0)
        {
            c = *str++;
            if ((c >= 48) && (c <= 57))         // 0-9
                c -= 48;
            else
                break;
            exp = SPAdd(SPFlt(c), SPMul(exp, g_10));
        }
        if (expSign)
            exp = SPMul(g_m1, exp);
        // _debug_puts ("str2f: exp="); _debug_putf(exp); _debug_putnl();
        f = SPMul(SPPow(exp, fBase), f);
    }

    //_debug_puts ("str2f: exponent handled, f="); _debug_putf(f); _debug_putnl();

    if (negative)
        f = SPMul(g_m1, f);

    //_debug_puts ("str2f: sign handled, f="); _debug_putf(f); _debug_putnl();

    return f;
}

FLOAT VAL_ (UBYTE *s)
{
    if (!s)
        return 0;

    LONG base=10, len=0;
    s = _val_handle_prefix(s, &base, &len);

    return _str2f_(s, len, base);
}

#ifdef __amigaos__
#pragma GCC push_options
#pragma GCC optimize ("O0")
#endif
void *memset (void *dst, register int c, register int n)
{
    if (n != 0)
	{
        register char *d = dst;

        do
            *d++ = c;
        while (--n != 0);
    }
    return (dst);
}
#ifdef __amigaos__
#pragma GCC pop_options
#endif

void _astr_init(void)
{
    g_positiveExpThreshold = SPFlt(10000000l);
    g_negativeExpThreshold = SPDiv(SPFlt(100000l), SPFlt(1));
    g_1e16  = SPPow(SPFlt( 16), SPFlt(10));
    g_1e9   = SPPow(SPFlt(  9), SPFlt(10));
    g_1e8   = SPPow(SPFlt(  8), SPFlt(10));
    g_1e4   = SPPow(SPFlt(  4), SPFlt(10));
    g_1e2   = SPPow(SPFlt(  2), SPFlt(10));
    g_1e1   = SPPow(SPFlt(  1), SPFlt(10));
    g_10    = SPFlt(10);
    g_1     = SPFlt(1);
    g_0     = SPFlt(0);
    g_m1    = SPFlt(-1);

    g_1en15 = SPPow(SPFlt(-15), SPFlt(10));
    g_1en7  = SPPow(SPFlt( -7), SPFlt(10));
    g_1en3  = SPPow(SPFlt( -3), SPFlt(10));
    g_1en1  = SPPow(SPFlt( -1), SPFlt(10));

    g_05    = SPDiv(SPFlt(2), SPFlt(1));
}

