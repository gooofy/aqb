
#include <stdio.h>
//#include <stdlib.h>
//#include <stdarg.h>
//#include <string.h>
//#include <errno.h>
#include <assert.h>
//#include <math.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#include <clib/mathtrans_protos.h>
#include <inline/mathtrans.h>

#include <_brt/_brt.h>

//#define DEBUG

ULONG _aqb_stack_size = 8192;

//  FFP Format

//    1mmm mmmm mmmm mmmm mmmm mmmm Seee eeee   (FFP )

// exponent is 7 bit excess-64 coded:
//
//  64         -64       -63  ...        0 ...       62        63
//        000 0000  000 0001      100 0000     111 1110  111 1111
//

FLOAT _mathffp_SPFlt ( register LONG integer __asm("d0") );

asm(
"__mathffp_SPFlt:                                                                 \n"

"         tst.l   d0                | zero?                                       \n"
"         beq     1f                | by convention FFP 0 == 0                    \n"

"         bmi.b   5f                | negative?                                   \n"
"         moveq   #95, d1           | set positive exponent  95 = 64+31 -> 2^31    \n"
"         bra     2f                | begin conversion                            \n"

"5:       neg.l   d0                | absolute value                              \n"
"         bvc     6f                | check for overflow                          \n"
"         moveq   #-32, d1          | overflow, exponent -32 = -128+64+32 -> 2^32 \n"
"         bra	  3f                | done                                        \n"
"6:       moveq   #-33, d1          | no overflow, exp   -33 = -128+64+31 -> 2^31 \n"

"         /* optimization: skip 16 normalization loop iters for small ints */     \n"
"2:       cmp.l   #0x7fff, d0       | >0x7fff ?                                   \n"
"         bhi     4f                | yes -> no optimization possible             \n"
"         swap    d0                | use swap to shift mantissa 16 bits          \n"
"         sub.b   #0x10, d1         | adjust exponent by 16                       \n"

"         /* normalization loop */                                                \n"
"4:       add.l   d0, d0            | shift mantissa left 1 bit                   \n"
"         dbmi    d1, 4b            | loop until MSB is set                       \n"

"		  /* rounding */                                                          \n"
"         tst.b   d0                | test low byte of mantissa                   \n"
"         bpl.b   3f                | MSB not set -> done                         \n"
"         add.l   #0x100, d0        | round up : mantissa + 1                     \n"
"         bcc.b   3f                | no overflow ? -> done                       \n"
"         roxr.l  #1, d0            | rotate down and insert carry bit            \n"
"         addq.b  #1, d1            | adjust exponent                             \n"

"3:       move.b  d1, d0            | put sign+exponent into d0                   \n"

"1:       rts                       |                                             \n"
);


/*
 * Lightweight float to string conversion
 *
 * source: https://blog.benoitblanchon.fr/lightweight-float-to-string/
 * author: Benoit Blanchon
 */

/* Motorola FFP format
 _____________________________________________
|                                             |
| MMMMMMMM    MMMMMMMM    MMMMMMMM    SEEEEEE |
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

    if (*value >= g_positiveExpThreshold)
    {
        if (*value >= g_1e16)
        {
            *value /= g_1e16;
            exponent += 16;
        }
        if (*value >= g_1e8)
        {
            *value /= g_1e8;
            exponent += 8;
        }
        if (*value >= g_1e4)
        {
            *value /= g_1e4;
            exponent += 4;
        }
        if (*value >= g_1e2)
        {
            *value /= g_1e2;
            exponent += 2;
        }
        if (*value >= g_1e1)
        {
            *value /= g_1e1;
            exponent += 1;
        }
    }

    if ( (g_0 > *value ) && (g_negativeExpThreshold <= *value) )
    {
        if (g_1en15 < *value)
        {
            *value *= g_1e16;
            exponent -= 16;
        }
        if (g_1en7 < *value)
        {
            *value *= g_1e8;
            exponent -= 8;
        }
        if (g_1en3 < *value)
        {
            *value *= g_1e4;
            exponent -= 4;
        }
        if (g_1en1 < *value)
        {
            *value *= g_1e2;
            exponent -= 2;
        }
        if (g_1 < *value)
        {
            *value *= g_1e1;
            exponent -= 1;
        }
    }

#ifdef DEBUG
    _debug_puts((STRPTR) "exponent:"); _debug_puts4(exponent); _debug_putnl();
#endif

    return exponent;
}

void _astr_ftoa_ext2(FLOAT value, UBYTE *buf, BOOL leading_space)
{
    BOOL negative = FALSE;

    if (value < g_0)
    {
        value = SPMul(value, g_m1);
        negative = TRUE;
#ifdef DEBUG
        _debug_puts((STRPTR) "negative.\n");
#endif
    }

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
    _debug_puts((STRPTR)"integralPart:"); _debug_puts4(integralPart); _debug_putnl();
#endif

    remainder *= g_1e9;
    decimalPart = SPFix(remainder);
#ifdef DEBUG
    _debug_puts((STRPTR)"decimalPart:"); _debug_puts4(decimalPart); _debug_putnl();
#endif

    // rounding
    remainder -= SPFlt(decimalPart);
    if (remainder >= g_05)
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

    _astr_itoa_ext(integralPart, &buf[0], 10, /*leading_space=*/ leading_space ? TRUE : negative);
    if (negative)
        buf[0] = '-';

    if (decimalPart)
    {
        int width = 9;

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

#define MAXBUF 256

void _debug_putf2(FLOAT f)
{
    UBYTE buf[MAXBUF];
    _astr_ftoa_ext2(f, buf, /*leading_space=*/TRUE);
    _debug_puts(buf);
}

void _analyze_ffp (FLOAT f)
{
    _debug_puts ((STRPTR) "_analyze_ffp: f=");
    _debug_putf2 (f);

	ULONG *ptr = (ULONG *) &f;
    _debug_puts ((STRPTR) ", FFP: 0x");
	_debug_puthex(*ptr);

	BYTE  e = ((*ptr) & 0x7f) - 64;

	_debug_puts((STRPTR) ", e=");
	_debug_puts1(e);

	ULONG m = (*ptr) >> 8;
	_debug_puts((STRPTR) ", m=0x");
	_debug_puthex(m);

    _debug_puts ((STRPTR) "\n");
}

int _aqb_main (int argc, char *argv[])
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

#if 0
	FLOAT f1 = _mathffp_SPFlt(-42);

    _analyze_ffp (f1);


#else
	for (int i=-10; i<10; i++)
	{
		FLOAT f1 = _mathffp_SPFlt(i*i*i*i*i*i*i*i);
		FLOAT f2 = SPFlt(i*i*i*i*i*i*i*i);

		_debug_puts((STRPTR) "f1=");
		_debug_putf2(f1);
		_debug_puts((STRPTR) " = ");
		ULONG *ptr = (ULONG *) &f1;
		_debug_putu4(*ptr);
		_debug_putnl();

		_debug_puts((STRPTR) "f2=");
		_debug_putf2(f2);
		_debug_puts((STRPTR) " = ");
		ptr = (ULONG *) &f2;
		_debug_putu4(*ptr);
		_debug_putnl();

        FLOAT f3 = g_0 -f1 - f2;
		_debug_puts((STRPTR) "f3=");
		_debug_putf2(f3);
		_debug_puts((STRPTR) " = ");
		ptr = (ULONG *) &f3;
		_debug_putu4(*ptr);
		_debug_putnl();


	}
#endif
    return 0;
}

