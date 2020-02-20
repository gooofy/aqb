/*
 * ___mulsi3
 *
 * source: libnix
 * license: public domain
 */
asm(
"		.globl	___mulsi3;"

/* D0 = D0 * D1 */

"___mulsi3:	moveml	sp@(4),d0/d1;"
"		movel	d3,sp@-;"
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
"		.globl	___udivsi3;"
"		.globl	___udivsi4;"

/* D1.L = D0.L % D1.L unsigned */

"___umodsi3:	moveml	sp@(4:W),d0/d1;"
"		jbsr	___udivsi4;"
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
"		.globl	___divsi3;"

/* D1.L = D0.L % D1.L signed */

"___modsi3:	moveml	sp@(4:W),d0/d1;"
"		jbsr	___divsi4;"
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
 * __pow_i4 / __pow_i2
 *
 * source: stackoverflow
 */

int __pow_i4(int base, int exp)
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

short __pow_i2(short base, short exp)
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

