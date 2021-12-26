#include "ratest.h"

#ifdef ENABLE_DPRINTF

#include <stdint.h>
#include <stdarg.h>
#include <string.h>

// a union to handle the
union _d_bits {
	double d;
	struct {
		unsigned sign :1;
		unsigned exp :11;
		unsigned frac0 :20;
		unsigned frac1 :32;
	} b;
	unsigned u;
};

#if 0
uint32_t strlen(const char *string)
{
    const char *s = string;

    while (*s++) {
    }
    return ~(string - s);
}
#endif

#define CHAR_BIT 8
#define MININTSIZE (sizeof(unsigned long long)*CHAR_BIT/3+1)
#define MINPOINTSIZE (sizeof(void *)*CHAR_BIT/4+1)
#define REQUIREDBUFFER (MININTSIZE>MINPOINTSIZE?MININTSIZE:MINPOINTSIZE)

/**
 * '#'
 * Used with o, exponent or X specifiers the value is preceeded with 0, 0x or 0X
 * respectively for values different than zero.
 * Used with a, A, e, E, f, F, g or G it forces the written output
 * to contain a decimal point even if no more digits follow.
 * By default, if no digits follow, no decimal point is written.
 */
#define ALTERNATEFLAG 1  /* '#' is set */

/**
 * '0'
 * Left-pads the number with zeroes (0) instead of spaces when padding is specified
 * (see width sub-specifier).
 */
#define ZEROPADFLAG   2  /* '0' is set */

/**
 * '-'
 * Left-justify within the given field width;
 * Right justification is the default (see width sub-specifier).
 */
#define LALIGNFLAG    4  /* '-' is set */

/**
 * ' '
 * If no sign is going to be written, a blank space is inserted before the value.
 */
#define BLANKFLAG     8  /* ' ' is set */

/**
 * '+'
 * Forces to preceed the result with a plus or minus sign (+ or -) even for positive numbers.
 * By default, only negative numbers are preceded with a - sign.
 */
#define SIGNFLAG      16 /* '+' is set */

static const char flagc[] = { '#', '0', '-', ' ', '+' };

static unsigned __ulldivus(unsigned long long * llp, unsigned short n) {
	struct LL {
		unsigned long hi;
		union {
			unsigned long lo;
			struct {
				unsigned short exponent;
				unsigned short y;
			} s;
		} u;
	}* hl = (struct LL *) llp;

	unsigned r;
	unsigned long h = hl->hi;
	if (h) {
		unsigned l = hl->u.s.exponent;
		unsigned k = hl->u.s.y;
		unsigned c = h % n;
		h = h / n;
		l = l + (c << 16);
		c = l % n;
		l = l / n;
		k = k + (c << 16);
		r = k % n;
		k = k / n;
		hl->u.lo = (l << 16) + k;
		hl->hi = h + (l >> 16);
		return r;
	}

	r = hl->u.lo % n;
	hl->u.lo /= n;
	return r;
}

const unsigned char __ctype[]=
{ 0x00,
  0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x28,0x28,0x28,0x28,0x28,0x20,0x20,
  0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
  0x88,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
  0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x10,0x10,0x10,0x10,0x10,0x10,
  0x10,0x41,0x41,0x41,0x41,0x41,0x41,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x10,0x10,0x10,0x10,0x10,
  0x10,0x42,0x42,0x42,0x42,0x42,0x42,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
  0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x10,0x10,0x10,0x10,0x20,
  0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
  0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
  0x08,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
  0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x10,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
  0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
  0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x10,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
  0x00,0x00,0x00
};

const unsigned char * const _ctype_=__ctype;

int isdigit(int c)
{
    return _ctype_[1+c]&4;
}

static void vdprintf(const char *format, va_list args)
{
	while (*format)
    {
		if (*format == '%') {
			static const char lowertabel[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
			static const char uppertabel[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
			short width = 0;
			unsigned short preci = 0x7fff;
			short flags = 0; /* Specifications */
			char type, subtype = 'i';
			char buffer1[2]; /* Signs and that like */
			char buffer[REQUIREDBUFFER]; /* The body */
			char *buffer2 = buffer; /* So we can set this to any other strings */
			uint32_t size1 = 0, size2 = 0; /* How many chars in buffer? */
			const char *ptr = format + 1; /* pointer to format string */
			unsigned short i, pad; /* Some temporary variables */

			do /* read flags */
				for (i = 0; i < sizeof(flagc); i++)
					if (flagc[i] == *ptr) {
						flags |= 1 << i;
						ptr++;
						break;
					} while (i < sizeof(flagc));

			if (*ptr == '*') /* read width from arguments */
			{
				signed int a;
				ptr++;
				a = va_arg(args, signed int);
				if (a < 0) {
					flags |= LALIGNFLAG;
					width = -a;
				} else
					width = a;
			} else {
				while (isdigit(*ptr))
					width = width * 10 + (*ptr++ - '0');
			}

			if (*ptr == 'h' || *ptr == 'l' || *ptr == 'L' || *ptr == 'j'
					|| *ptr == 'z' || *ptr == 't') {
				subtype = *ptr++;
				if (*ptr == 'h' || *ptr == 'l')
					++ptr, ++subtype;
			} else
				subtype = 0;

			type = *ptr++;

			switch (type) {
			case 'd':
			case 'i':
			case 'o':
			case 'p':
			case 'u':
			case 'x':
			case 'X': {
				unsigned long long v;
				const char *tabel;
				int base;

				if (type == 'p') {
					subtype = 'l'; /* This is written as %#lx */
					type = 'x';
					flags |= ALTERNATEFLAG;
				}

				if (type == 'd' || type == 'i') /* These are signed */
				{
					signed long long v2;
					if (subtype == 'l')
						v2 = va_arg(args, signed long);
					else if (subtype == 'm' || subtype == 'j')
						v2 = va_arg(args, signed long long);
					else
						v2 = va_arg(args, signed int);
					if (v2 < 0) {
						buffer1[size1++] = '-';
						v = -v2;
					} else {
						if (flags & SIGNFLAG)
							buffer1[size1++] = '+';
						else if (flags & BLANKFLAG)
							buffer1[size1++] = ' ';
						v = v2;
					}
				} else /* These are unsigned */
				{
					if (subtype == 'l')
						v = va_arg(args, unsigned long);
					else if (subtype == 'm' || subtype == 'j')
						v = va_arg(args, unsigned long long);
					else
						v = va_arg(args, unsigned int);
					if (flags & ALTERNATEFLAG) {
						if (type == 'o') {
							if (!preci || v)
								buffer1[size1++] = '0';
						} else if ((type == 'x' || type == 'X') && v) {
							buffer1[size1++] = '0';
							buffer1[size1++] = type;
						}
					}
				}

				buffer2 = &buffer[sizeof(buffer)]; /* Calculate body string */
				base = type == 'x' || type == 'X' ? 16 : (type == 'o' ? 8 : 10);
				tabel = type != 'X' ? lowertabel : uppertabel;
				do {
					*--buffer2 = tabel[__ulldivus(&v, base)];
					size2++;
				} while (v);
				if (preci == 0x7fff) /* default */
					preci = 0;
				else
					flags &= ~ZEROPADFLAG;
				break;
			}
			case 'c':
				if (subtype == 'l')
					*buffer2 = va_arg(args, long);
				else
					*buffer2 = va_arg(args, int);
				size2 = 1;
				preci = 0;
				break;
			case 's':
				buffer2 = va_arg(args, char *);
				size2 = strlen(buffer2);
				size2 = size2 <= preci ? size2 : preci;
				preci = 0;
				break;

			case '%':
				buffer2 = "%";
				size2 = 1;
				preci = 0;
				break;
			default:
				if (!type)
					ptr--; /* We've gone too far - step one back */
				buffer2 = (char *) format;
				size2 = ptr - format;
				width = preci = 0;
				break;
			}

			pad = size1 + (size2 >= preci ? size2 : preci); /* Calculate the number of characters */
			pad = pad >= width ? 0 : width - pad; /* and the number of resulting pad bytes */

			if (flags & ZEROPADFLAG) /* print sign and that like */
				for (i = 0; i < size1; i++)
					_debug_putc(buffer1[i]);

			if (!(flags & LALIGNFLAG)) /* Pad left */
				for (i = 0; i < pad; i++)
					_debug_putc(flags&ZEROPADFLAG?'0':' ');

			if (!(flags & ZEROPADFLAG)) /* print sign if not zero padded */
				for (i = 0; i < size1; i++)
					_debug_putc(buffer1[i]);

			for (i = size2; i < preci; i++) /* extend to precision */
				_debug_putc('0');

			for (i = 0; i < size2; i++) /* print body */
				_debug_putc(buffer2[i]);

			if (flags & LALIGNFLAG) /* Pad right */
				for (i = 0; i < pad; i++)
					_debug_putc(' ');

			format = ptr;
		} else
			_debug_putc(*format++);
	}
}

void dprintf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vdprintf(format, args);
    va_end(args);
}

#endif
