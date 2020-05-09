/*
 * unused, for now
 */

/*
 * amiga exec like defines (do _NOT_ use on a real amiga, use the exec ones instead!)
 */

typedef int16_t		    SHORT;
typedef uint16_t		USHORT;

typedef int32_t			LONG;
typedef uint32_t		ULONG;

typedef uint32_t        FLOAT;

#define DEBUG

static void _aio_puts(char *s)
{
	printf("%s", s);
}

static void _aio_puts4(LONG i)
{
	printf("%d", i);
}

static void _aio_putnl(void)
{
	printf("\n");
}
/*
 * amiga FFP emulation (testing purposes only)
 */

static FLOAT SPDiv (FLOAT a, FLOAT b)
{
	float fa, fb, res;

	fa = decode_ffp(a);
	fb = decode_ffp(b);

	res = fb / fa;

#ifdef DEBUG
	printf ("SPDiv: a=%f, b=%f -> res = b/a=%f\n", fa, fb, res);
#endif

	return encode_ffp(res);
}

static FLOAT SPFlt(LONG l)
{
	return encode_ffp((float) l);
}

static FLOAT SPPow (FLOAT a, FLOAT b)
{
	return encode_ffp(powf(decode_ffp(b), decode_ffp(a)));
}

static FLOAT g_positiveExpThreshold;
static FLOAT g_negativeExpThreshold;
static FLOAT g_1e16, g_1e8, g_1e4, g_1e2, g_1e1, g_1e9, g_0, g_1;
static FLOAT g_1en15, g_1en7, g_1en3, g_1en1, g_05, g_m1;

static void _astr_init(void)
{
    g_positiveExpThreshold = SPFlt(10000000l);
    g_negativeExpThreshold = SPDiv(SPFlt(100000l), SPFlt(1));
    g_1e16  = SPPow(SPFlt( 16), SPFlt(10));
    g_1e9   = SPPow(SPFlt(  9), SPFlt(10));
    g_1e8   = SPPow(SPFlt(  8), SPFlt(10));
    g_1e4   = SPPow(SPFlt(  4), SPFlt(10));
    g_1e2   = SPPow(SPFlt(  2), SPFlt(10));
    g_1e1   = SPPow(SPFlt(  1), SPFlt(10));
    g_1     = SPFlt(1);
    g_0     = SPFlt(0);
    g_m1    = SPFlt(-1);

    g_1en15 = SPPow(SPFlt(-15), SPFlt(10));
    g_1en7  = SPPow(SPFlt( -7), SPFlt(10));
    g_1en3  = SPPow(SPFlt( -3), SPFlt(10));
    g_1en1  = SPPow(SPFlt( -1), SPFlt(10));

    g_05    = SPDiv(SPFlt(2), SPFlt(1));
}

// normalizes the value between 1e-5 and 1e7 and returns the exponent
static SHORT normalizeFFP(FLOAT *value)
{
	ULONG ui = *((ULONG *) value);

    SHORT e = (ui & 0x0000007f) - 65;
	SHORT exponent = 0;

#ifdef DEBUG
	_aio_puts ("normalizeFFP: e is "); _aio_puts4(e); _aio_putnl();
#endif

	if (e >= 16)
	{
		exponent += 16;
		e -= 16;
		*value = SPDiv(g_1e16, *value);
	}

#ifdef DEBUG
	_aio_puts ("normalizeFFP: exponent is "); _aio_puts4(exponent); _aio_putnl();
#endif

#if 0
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
#endif 

    return exponent;
}


