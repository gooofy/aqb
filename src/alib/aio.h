#ifndef HAVE_AIO_H
#define HAVE_AIO_h

#include <exec/types.h>
#include <clib/dos_protos.h>

extern BPTR g_stdout;

void _aio_init(void);
void _aio_shutdown(void);

void _aio_puts4(int num);
void _aio_puts2(short num);
void _aio_puthex(int num);
void _aio_putuhex(ULONG l);
void _aio_putbin(int num);
void _aio_putf(FLOAT f);
void _aio_putbool(BOOL b);

void _aio_puts(const char *str);

void _aio_putnl(void);
void _aio_puttab(void);

#endif

