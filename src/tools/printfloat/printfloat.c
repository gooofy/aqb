
#include <stdio.h>
//#include <stdlib.h>
//#include <stdarg.h>
//#include <string.h>
//#include <errno.h>
#include <assert.h>
#include <math.h>

static void float2str(double v, char *buffer, int buf_len)
{
    const char *	buffer_stop		= &buffer[buf_len-1];
    char *			buffer_start	= buffer;

    const char * digit_encoding = "0123456789abcdef";
    const int radix = 10;

    char *output_buffer = buffer_start;
    int precision = 6;

    if (isinf(v))
    {
        if (v < 0)
            (*output_buffer++) = '-';
        strcpy(output_buffer,"inf");
    }
    else if (isnan(v))
    {
        strcpy(output_buffer,"nan");
    }
    else
    {
        double roundoff_fudge = 0.0;
        int num_leading_digits;
        int max_digits = -1;
        int digit;

        if (v < 0.0)
        {
            (*output_buffer++) = '-';
            v = (-v);
        }

        roundoff_fudge = pow((double)radix,(double)(precision + 1));

        if(roundoff_fudge > 0.0)
            v += 5.0 / roundoff_fudge;

        // compute number of leading digits

        double v2 = v;
        if (v2 < radix)
        {
            num_leading_digits = 1;
        }
        else
        {
            num_leading_digits = 0;

            while(floor(v2) > 0.0)
            {
                num_leading_digits++;

                v2 /= radix;
            }
        }

        if (v >= 1.0)
        {
            /* 'Normalize' the number so that we have a zero in
               front of the mantissa. We can't lose here: we
               simply scale the value without any loss of
               precision (we just change the floating point
               exponent). */
            v /= pow((double)radix,(double)num_leading_digits);

            for (int i = 0 ; (max_digits != 0) && (i < num_leading_digits) && (output_buffer < buffer_stop) ; i++)
            {
                v *= radix;

                digit = floor(v);

                (*output_buffer++) = digit_encoding[digit];

                v -= digit;

                if(max_digits > 0)
                    max_digits--;
            }
        }
        else
        {
            /* NOTE: any 'significant' digits (for %g conversion)
                     will follow the decimal point. */
            (*output_buffer++) = '0';
        }

        /* Now for the fractional part. */
		if (output_buffer < buffer_stop)
		{
			(*output_buffer++) = '.';

			int i;

			for (i = 0 ; (i < precision) && (output_buffer < buffer_stop) ; i++)
			{
				v *= radix;

				digit = floor(v);

				(*output_buffer++) = digit_encoding[digit];

				v -= digit;
			}

			/* Strip trailing digits and decimal point */
			while(output_buffer > buffer_start+1 && output_buffer[-1] == '0')
				output_buffer--;

			if(output_buffer > buffer_start && output_buffer[-1] == '.')
				output_buffer--;
		}

        *output_buffer = '\0';
    }
}


int main (int argc, char *argv[])
{
    char buf[1024];

    float f = 3.14159;

    float2str (f, buf, 1024); printf ("buf=%s\n", buf);
    float2str (-23, buf, 1024); printf ("buf=%s\n", buf);
    float2str (42, buf, 1024); printf ("buf=%s\n", buf);
    //printf ("f=%g, buf=%s\n", f, buf);

    return 0;
}

