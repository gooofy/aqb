
#include <stdio.h>
//#include <stdlib.h>
//#include <stdarg.h>
//#include <string.h>
//#include <errno.h>
#include <assert.h>
//#include <math.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#include <clib/mathtrans_protos.h>
#include <inline/mathtrans.h>

#include <_brt/_brt.h>

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



int _aqb_main (int argc, char *argv[])
{
#if 0
	FLOAT f1 = _mathffp_SPFlt(42);
	ULONG *ptr = (ULONG *) &f1;
	_debug_putu4(*ptr);
	_debug_putnl();

	ULONG e = (*ptr) & 0xff;
	BYTE  es = e;
	ULONG m = (*ptr) >> 8;

	_debug_puts((STRPTR) "e=0x");
	_debug_puthex(e);
	_debug_puts((STRPTR) "=");
	_debug_puts1(es);

	_debug_puts((STRPTR) "=>");
	if (es>0)
		_debug_puts1(es-64);
	else
		_debug_puts1(e-0x80-64);

	_debug_puts((STRPTR) ", m=0x");
	_debug_puthex(m);
	_debug_putnl();



#else
	for (int i=-10; i<10; i++)
	{
		FLOAT f1 = _mathffp_SPFlt(i*i*i*i*i*i*i*i);
		FLOAT f2 = SPFlt(i*i*i*i*i*i*i*i);

		_debug_puts((STRPTR) "f1=");
		_debug_putf(f1);
		_debug_puts((STRPTR) " = ");
		ULONG *ptr = (ULONG *) &f1;
		_debug_putu4(*ptr);
		_debug_putnl();

		_debug_puts((STRPTR) "f2=");
		_debug_putf(f2);
		_debug_puts((STRPTR) " = ");
		ptr = (ULONG *) &f2;
		_debug_putu4(*ptr);
		_debug_putnl();
	}
#endif
    return 0;
}

