#include "_brt.h"

#include <exec/types.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

/*
 * ___mulsi3
 *
 * source: libnix
 * license: public domain
 */
asm(
"		.globl	___mulsi3;"
"		.globl	___mulsi4;"

/* D0 = D0 * D1 */

"___mulsi3:	moveml	sp@(4),d0/d1;"
"___mulsi4: movel	d3,sp@-;"
"		movel	d2,sp@-;"
"		movew	d1,d2;"
"		mulu	d0,d2;"
"		movel	d1,d3;"
"		swap	d3;"
"		mulu	d0,d3;"
"		swap	d3;"
"		clrw	d3;"
"		addl	d3,d2;"
"		swap	d0;"
"		mulu	d1,d0;"
"		swap	d0;"
"		clrw	d0;"
"		addl	d2,d0;"
"		movel	sp@+,d2;"
"		movel	sp@+,d3;"
"		rts;"
);

/*
 * ___udivsi3
 * ___umodsi3
 *
 * source: libnix
 * license: public domain
 */
asm(
"		.globl	___umodsi3;"
"		.globl	___umodsi4;"
"		.globl	___udivsi3;"
"		.globl	___udivsi4;"

/* D1.L = D0.L % D1.L unsigned */

"___umodsi3:	moveml	sp@(4:W),d0/d1;"
"___umodsi4:	jbsr	___udivsi4;"
"		movel	d1,d0;"
"		rts;"

/* D0.L = D0.L / D1.L unsigned */

"___udivsi3:	moveml	sp@(4:W),d0/d1;"
"___udivsi4:	movel	d3,sp@-;"
"		movel	d2,sp@-;"
"		movel	d1,d3;"
"		swap	d1;"
"		tstw	d1;"
"		bnes	LC104;"
"		movew	d0,d2;"
"		clrw	d0;"
"		swap	d0;"
"		divu	d3,d0;"
"		movel	d0,d1;"
"		swap	d0;"
"		movew	d2,d1;"
"		divu	d3,d1;"
"		movew	d1,d0;"
"		clrw	d1;"
"		swap	d1;"
"		jra	LC101;"
"LC104:		movel	d0,d1;"
"		swap	d0;"
"		clrw	d0;"
"		clrw	d1;"
"		swap	d1;"
"		moveq	#16-1,d2;"
"LC103:		addl	d0,d0;"
"		addxl	d1,d1;"
"		cmpl	d1,d3;"
"		bhis	LC102;"
"		subl	d3,d1;"
"		addqw	#1,d0;"
"LC102:		dbra	d2,LC103;"
"LC101:		movel	sp@+,d2;"
"		movel	sp@+,d3;"
"		rts;"
);

/*
 * ___divsi3
 *
 * source: libnix
 * license: public domain
 */

asm(
"		.globl	_div;"
"		.globl	_ldiv;"
"		.globl	___modsi3;"
"		.globl	___modsi4;"
"		.globl	___divsi3;"
"		.globl	___divsi4;"

/* D1.L = D0.L % D1.L signed */

"___modsi3:	moveml	sp@(4:W),d0/d1;"
"___modsi4:	jbsr	___divsi4;"
"		movel	d1,d0;"
"		rts;"

/* D0.L = D0.L / D1.L signed */

"_div:;"
"_ldiv:;"
"___divsi3:	moveml	sp@(4:W),d0/d1;"
"___divsi4:	movel	d3,sp@-;"
"		movel	d2,sp@-;"
"		moveq	#0,d2;"
"		tstl	d0;"
"		bpls	LC5;"
"		negl	d0;"
"		addql	#1,d2;"
"LC5:		movel	d2,d3;"
"		tstl	d1;"
"		bpls	LC4;"
"		negl	d1;"
"		eoriw	#1,d3;"
"LC4:		jbsr	___udivsi4;"
"LC3:		tstw	d2;"
"		beqs	LC2;"
"		negl	d0;"
"LC2:		tstw	d3;"
"		beqs	LC1;"
"		negl	d1;"
"LC1:		movel	sp@+,d2;"
"		movel	sp@+,d3;"
"		rts;"
);

/*
 * __pow_s4 / __pow_s2
 *
 * source: stackoverflow
 */

int __pow_s4(int base, int exp)
{
    int result = 1;
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

short __pow_s2(short base, short exp)
{
    short result = 1;
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

char __pow_s1(char base, char exp)
{
    char result = 1;
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

char __mul_s1(char a, char b)
{
    return a*b;
}

char __div_s1(char a, char b)
{
    return a/b;
}

char __mod_s1(char a, char b)
{
    return a%b;
}

unsigned char __pow_u1(unsigned char base, unsigned char exp)
{
    unsigned char result = 1;
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

unsigned char __mul_u1(unsigned char a, unsigned char b)
{
    return a*b;
}

unsigned char __div_u1(unsigned char a, unsigned char b)
{
    return a/b;
}

unsigned char __mod_u1(unsigned char a, unsigned char b)
{
    return a%b;
}

static FLOAT g_one_half, g_zero;

FLOAT __aqb_mod(FLOAT divident, FLOAT divisor)
{
    // this is not what quickbasic does
    // FLOAT q = SPFlt(SPFix(SPDiv(divident, divisor)));
    // return SPSub(divident, SPMul(q, divisor));

    // instead, it just rounds the two operands

    LONG a = clng_(divident);
    LONG b = clng_(divisor);

    return SPFlt(a % b);
}

/*
    int64 qbr(long double f){
        int64 i; int temp=0;
        if (f>9223372036854775807) {temp=1;f=f-9223372036854775808u;} //if it's too large for a signed int64, make it an unsigned int64 and return that value if possible.
        if (f<0) i=f-0.5f; else i=f+0.5f;
        if (temp) return i|0x8000000000000000;//+9223372036854775808;
        return i;
    }
*/


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

void _amath_init(void)
{
    g_one_half = SPDiv(SPFlt(2), SPFlt(1));
    g_zero     = SPFlt(0);
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
