#include "astr.h"
#include "autil.h"

#include <exec/memory.h>
#include <clib/exec_protos.h>

#include <clib/mathffp_protos.h>
#include <clib/mathtrans_protos.h>

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

void _astr_itoa(int num, char* str, int base)
{
    int i = 0;
    BOOL isNegative = FALSE;

    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0)
    {
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

    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    reverse(str, i);
}

ULONG _astr_len(const char *str)
{
    int l = 0;
    while (*str)
    {
        str++;
        l++;
    }
    return l;
}

char *_astr_dup(const char* str)
{
    ULONG l = _astr_len(str);
    char *str2 = _autil_alloc(l+1, MEMF_ANY);
    CopyMem((APTR)str, (APTR)str2, l+1);
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
static FLOAT g_1e16, g_1e8, g_1e4, g_1e2, g_1e1, g_1e9, g_0, g_1;
static FLOAT g_1en15, g_1en7, g_1en3, g_1en1, g_05;

// normalizes the value between 1e-5 and 1e7 and returns the exponent
static SHORT normalizeFloat(FLOAT *value) {

    SHORT exponent = 0;

    if (SPCmp(*value, g_positiveExpThreshold) >=0)
    {
        if (SPCmp(*value, g_1e16) >= 0)
        {
            *value = SPDiv(*value, g_1e16);
            exponent += 16;
        }
        if (SPCmp(*value, g_1e8) >= 0)
        {
            *value = SPDiv(*value, g_1e8);
            exponent += 8;
        }
        if (SPCmp(*value, g_1e4) >= 0)
        {
            *value = SPDiv(*value, g_1e4);
            exponent += 4;
        }
        if (SPCmp(*value, g_1e2) >= 0) {
            *value = SPDiv(*value, g_1e2);
            exponent += 2;
        }
        if (SPCmp(*value, g_1e1) >= 0)
        {
            *value = SPDiv(*value, g_1e1);
            exponent += 1;
        }
    }

    if ( (SPCmp(*value,0) > 0) && (SPCmp(*value, g_negativeExpThreshold) <= 0) ) {
        if (SPCmp(*value, g_1en15) <0) {
            *value = SPMul(*value, g_1e16);
            exponent -= 16;
        }
        if (SPCmp(*value, g_1en7) <0) {
            *value  = SPMul(*value, g_1e8);
            exponent -= 8;
        }
        if (SPCmp(*value, g_1en3) <0) {
            *value  = SPMul(*value, g_1e4);
            exponent -= 4;
        }
        if (SPCmp(*value, g_1en1) <0) {
            *value  = SPMul(*value, g_1e2);
            exponent -= 2;
        }
        if (SPCmp(*value, g_1) <0) {
            *value  = SPMul(*value, g_1e1);
            exponent -= 1;
        }
    }

    return exponent;
}

void _astr_ftoa(FLOAT value, char *buf)
{
    BOOL negative = FALSE;

    // if (isnan(value)) return _aio_puts("nan");

    if (SPCmp(value, 0.0)<0)
    {
        value = SPMul(value, SPFlt(-1));
        negative = TRUE;
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
    FLOAT remainder = SPSub(value, SPFlt(integralPart));

    remainder = SPMul(remainder, g_1e9);
    decimalPart = SPFix(remainder);

    // rounding
    remainder = SPSub(remainder, SPFlt(decimalPart));
    if (SPCmp(remainder, g_05) >=0)
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

    if (negative)
        buf[0] = '-';
    else
        buf[0] = ' ';

    _astr_itoa(integralPart, &buf[1], 10);

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

        char buffer[16];
        char *ptr = buffer + sizeof(buffer) - 1;

        // write the string in reverse order
        *ptr = 0;
        while (width--)
        {
            *--ptr = decimalPart % 10 + '0';
            decimalPart /= 10;
        }
        *--ptr = '.';

        // and dump it in the right order
        ULONG l = _astr_len(buf);
        CopyMem((APTR) ptr, (APTR) &buf[l], _astr_len(ptr)+1);
    }

    if (exponent != 0)
    {
        ULONG l = _astr_len(buf);
        buf[l] = 'e'; buf[l+1] = '-';
        _astr_itoa(exponent, &buf[l+1], 10);
    }
}

void _astr_init(void)
{
    g_positiveExpThreshold = SPFlt(10000000l);
    g_negativeExpThreshold = SPDiv(SPFlt(1), SPFlt(100000l));
    g_1e16  = SPPow(SPFlt( 16), SPFlt(10));
    g_1e9   = SPPow(SPFlt(  9), SPFlt(10));
    g_1e8   = SPPow(SPFlt(  8), SPFlt(10));
    g_1e4   = SPPow(SPFlt(  4), SPFlt(10));
    g_1e2   = SPPow(SPFlt(  2), SPFlt(10));
    g_1e1   = SPPow(SPFlt(  1), SPFlt(10));
    g_1     = SPFlt(1);
    g_0     = SPFlt(0);

    g_1en15 = SPPow(SPFlt(-15), SPFlt(10));
    g_1en7  = SPPow(SPFlt( -7), SPFlt(10));
    g_1en3  = SPPow(SPFlt( -3), SPFlt(10));
    g_1en1  = SPPow(SPFlt( -1), SPFlt(10));

    g_05    = SPDiv(SPFlt(1), SPFlt(2));
}

