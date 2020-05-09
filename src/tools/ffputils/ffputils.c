#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>


static unsigned int encode_ffp(float f)
{
    unsigned int res, fl;

    fl = *((unsigned int *) &f);

    if (f==0.0)
        return 0;

    // exponent
    res = (fl & 0x7F800000) >> 23;
    res = res - 126 + 0x40;

    // overflow
    if ((char) res < 0)
        return res;

    // mantissa
    res |= (fl << 8) | 0x80000000;

    // sign
    if (f < 0)
        res |= 0x00000080;

    return res;
}

static float decode_ffp(unsigned int fl)
{
    unsigned int res;

    if (fl == 0)
        return 0.0;

    // exponent

    // 0x    7    F    8    0    0    0    0    0
    //    0000 0000 0000 0000 0000 0000 Seee eeee   (FFP )
    //    0eee eeee e000 0000 0000 0000 0000 0000   (IEEE)

    res = (fl & 0x0000007f) + 126 - 0x40;
    res = res << 23;

    // printf ("exponent converted: 0x%08x\n", res);

    // mantissa

    //    1mmm mmmm mmmm mmmm mmmm mmmm Seee eeee   (FFP )
    //    0eee eeee emmm mmmm mmmm mmmm mmmm mmmm   (IEEE)

    res |= ((fl & 0x7fffff00) >> 8);

    // printf ("mantissa converted: 0x%08x\n", res);

    // sign
    if (fl & 0x80)
        res |= 0x80000000;

    // printf ("sign     converted: 0x%08x\n", res);

    return *((float *)&res);
}

static void disassemble_ffp(unsigned int ui)
{
    printf("FFP: 0x%08x\n", ui);
    printf("       MMMMMMSE\n");

    if (!ui)
    {
        printf("       zero\n");
        return;
    }

    char sign = ui & 0x80;
    if (sign)
        printf("       negative\n");
    else
        printf("       positive\n");

    unsigned int m = (ui & 0xffffff00) >> 8;
    float mf = ((float) m) / 8388608.0;
    printf("       mantissa: 0x%08x = %d -> %f\n", m, m, mf);

    unsigned int   e  = ui & 0x0000007f;
    float ef = (float) e - 65.0;
    printf("       exponent: 0x%08x = %d -> %f\n", e, e, ef);
}

static void roundtrip_test(float f)
{
    unsigned int ieee1 = *((unsigned int*) &f);
    unsigned int ui    = encode_ffp(f);
    float        f2    = decode_ffp(ui);
    unsigned int ieee2 = *((unsigned int*) &f2);

    disassemble_ffp(ui);
    printf ("%f (IEEE: 0x%08x) -> [ENCODER] -> FFP: 0x%08x -> [DECODER] -> %f (IEEE: 0x%08x)\n", f, ieee1, ui, f2, ieee2);
}

static void decode_test(unsigned int ui)
{
    float        f2    = decode_ffp(ui);
    unsigned int ieee2 = *((unsigned int*) &f2);

    disassemble_ffp(ui);

    printf ("FFP: 0x%08x -> [DECODER] -> %f (IEEE: 0x%08x)\n", ui, f2, ieee2);
}

int main (int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf (stderr, "usage: %s value\n", argv[0]);
        fprintf (stderr, "   value: floating point number (e.g. 42.23)      -> roundtrip test\n");
        fprintf (stderr, "   value: hex number            (e.g. 0xc8f5c342)-> FFP decode test\n");
        exit(1);
    }

    char *s = argv[1];
    if (s[0] == '0' && s[1]=='x')
    {
        decode_test((int)strtol(&s[2], NULL, 16));
    }
    else
    {
        roundtrip_test(atof(s));
    }

    return 0;
}

