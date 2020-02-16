
#include "atoi.h"

#define MAXBUF 40

void lowlevel_puts(char *str);

void puts4(int num)
{
    char buf[MAXBUF];

    if (num<0)
    {
        lowlevel_puts("-");
        num *= -1;
    }
    else
    {
        lowlevel_puts(" ");
    }

    itoa(num, buf, 10);

    lowlevel_puts(buf);
}

void puts2(short num)
{
    char buf[MAXBUF];

    if (num<0)
    {
        lowlevel_puts("-");
        num *= -1;
    }
    else
    {
        lowlevel_puts(" ");
    }

    itoa(num, buf, 10);

    lowlevel_puts(buf);
}

short baz (void);

void bar(short a, short b, short c, short d, short e)
{
    puts2(a);
    puts2(b);
    puts2(c);
    puts2(d);
    puts2(e);
}

void foo(void)
{
    short s, t;

    s = baz();
    t = baz();

    puts2(s);
    bar(s,t,3,4,5);
}

void puthex(int num)
{
    char buf[MAXBUF];

    itoa(num, buf, 16);

    lowlevel_puts(buf);
}

void putbin(int num)
{
    char buf[MAXBUF];

    itoa(num, buf, 2);

    lowlevel_puts(buf);
}

