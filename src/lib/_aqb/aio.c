
#include "../_brt/_brt.h"
#include "_aqb.h"

#include <clib/dos_protos.h>
#include <inline/dos.h>

#define MAXBUF 40

void _aio_putnl(USHORT fno)
{
    _aio_puts(fno, (UBYTE*)"\n");
}

void _aio_puts4(USHORT fno, LONG num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _aio_puts(fno, buf);
}

void _aio_puts2(USHORT fno, short num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _aio_puts(fno, buf);
}

void _aio_puts1(USHORT fno, UBYTE num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _aio_puts(fno, buf);
}

void _aio_putu4(USHORT fno, ULONG num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa(num, buf, 10);

    _aio_puts(fno, buf);
}

void _aio_putu2(USHORT fno, USHORT num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa(num, buf, 10);

    _aio_puts(fno, buf);
}

void _aio_putu1(USHORT fno, UBYTE num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa(num, buf, 10);

    _aio_puts(fno, buf);
}

void _aio_puthex(USHORT fno, LONG num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 16);

    _aio_puts(fno, buf);
}

void _aio_putbin(USHORT fno, LONG num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 2);

    _aio_puts(fno, buf);
}

void _aio_putuhex(USHORT fno, ULONG l)
{
    UBYTE buf[MAXBUF];
    ULONG digit;

    for (int i = 7; i>=0; i--)
    {
        digit = (l >> (4*i)) & 0xf;

        if (digit < 10)
            buf[7-i] = '0' + digit;
        else
            buf[7-i] = 'a' + (digit-10);
    }
    buf[8] = 0;

    _aio_puts(fno, buf);
}

void _aio_putf(USHORT fno, FLOAT f)
{
    UBYTE buf[40];
    _astr_ftoa(f, buf);
    _aio_puts(fno, buf);
}

void _aio_putbool(USHORT fno, BOOL b)
{
    _aio_puts(fno, b ? (UBYTE*)"TRUE" : (UBYTE*)"FALSE");
}

/*********************************************************
 *
 * [LINE] INPUT support
 *
 *********************************************************/

void _aio_line_input (UBYTE *prompt, UBYTE **s, BOOL do_nl)
{
    DPRINTF ("_aio_line_input: prompt=%s, *s=0x%08lx, do_nl=%d\n", prompt ? (char *) prompt : "NULL", *s, do_nl);

    if (prompt)
        _aio_puts(/*FIXME*/0, prompt);

    _aio_gets(s, do_nl);
}

#define MAX_TOKEN_LEN 1024

static UBYTE *g_input_buffer;
static LONG   g_input_pos;
static LONG   g_input_len;
static BOOL   g_input_eof;
static UBYTE  g_input_ch;
static UBYTE  g_input_token[MAX_TOKEN_LEN+1]; // add room for final \0

static void _input_getch (void)
{
    if (g_input_pos >= g_input_len)
    {
        g_input_eof = TRUE;
        return;
    }
    g_input_ch = g_input_buffer[g_input_pos++];
}

static BOOL _input_is_whitespace(void)
{
    return (g_input_ch == ' ') || (g_input_ch == '\t');
}

static void _input_skip_whitespace (void)
{
    while (_input_is_whitespace () && !g_input_eof)
        _input_getch ();
}

static void _input_skip_delimiter (void)
{
    _input_skip_whitespace();

    if (g_input_eof)
        return;

    BOOL delim_found = FALSE;
    while (!delim_found && !g_input_eof)
    {
        switch (g_input_ch)
        {
            case ',':
            case '\n':
                _input_getch();
                delim_found = TRUE;
                break;

            case '\r':
                _input_getch();
                if (g_input_ch == '\n')
                {
                    _input_getch();
                    delim_found = TRUE;
                }
                break;
            default:
                _input_getch();
                break;
        }
    }
}

static void _input_next_token (void)
{
    DPRINTF("_input_next_token: skip whitespace...\n");
    _input_skip_whitespace();
    if (g_input_eof)
        ERROR (AE_INPUT_OUT_OF_DATA); // FIXME: in console input, re-prompt the user

    BOOL skip_delim = TRUE;
    BOOL in_quote   = FALSE;
    int  len        = 0;

    while (!g_input_eof && (len < MAX_TOKEN_LEN))
    {
        DPRINTF("_input_next_token: token... (g_input_ch=%c[%d])\n", g_input_ch, g_input_ch);
        switch (g_input_ch)
        {
            case '\n':
                skip_delim = FALSE;
                goto fini;

            case '\r':
                _input_getch();
                if (g_input_ch == '\n')
                {
                    _input_getch();
                    skip_delim = FALSE;
                    goto fini;
                }
                break;

            case '"':
                if (in_quote)
                {
                    _input_getch();
                    goto fini;
                }

                if (len == 0)
                {
                    in_quote = TRUE;
                    _input_getch();
                    continue;
                }
                break;

            case ',':
                if (!in_quote)
                {
                    _input_getch();
                    skip_delim = FALSE;
                    goto fini;
                }
                break;

            case '\t':
            case ' ':
                if (!in_quote)
                {
                    _input_getch();
                    continue;
                }
                break;

            default:
                break;
        }

        g_input_token[len++] = g_input_ch;
        _input_getch();
    }

fini:
    g_input_token[len++] = '\0';
    DPRINTF("_input_next_token: fini g_input_token=%s\n", g_input_token);

    // _aio_puts("delim\n");
    if (skip_delim)
        _input_skip_delimiter ();
}

void _aio_console_input (BOOL qm, UBYTE *prompt, BOOL do_nl)
{
    UBYTE *p = qm ? prompt : (UBYTE*) "?";
    _aio_line_input (p, &g_input_buffer, do_nl);
    g_input_pos = 0;
    g_input_len = LEN_(g_input_buffer);
    g_input_eof = FALSE;
    _input_getch();
}

void _aio_inputs1 (BYTE   *v)
{
    _input_next_token ();
    *v = VALINT_ (g_input_token);
}
void _aio_inputu1 (UBYTE  *v)
{
    _input_next_token ();
    *v = VALINT_ (g_input_token);
}
void _aio_inputs2 (SHORT  *v)
{
    _input_next_token ();
    *v = VALINT_ (g_input_token);
}
void _aio_inputu2 (USHORT *v)
{
    _input_next_token ();
    *v = VALUINT_ (g_input_token);
}
void _aio_inputs4 (LONG   *v)
{
    _input_next_token ();
    *v = VALLNG_ (g_input_token);
}
void _aio_inputu4 (ULONG  *v)
{
    _input_next_token ();
    *v = VALULNG_ (g_input_token);
}
void _aio_inputf  (FLOAT  *v)
{
    _input_next_token ();
    *v = VAL_ (g_input_token);
}
void _aio_inputs (UBYTE **v)
{
    _input_next_token ();
    *v = _astr_dup(g_input_token);
}

#define AIO_MAX_FILES 16

static struct FileHandle *g_files[AIO_MAX_FILES];

void _aio_open (UBYTE *fname, USHORT mode, USHORT access, USHORT fno, USHORT recordlen)
{
#if 0
    _aio_puts(0, (STRPTR)"_aio_open: fname=")  ; _aio_puts(0, fname);
    _aio_puts(0, (STRPTR)", mode: "); _aio_puts2(0, mode);
    _aio_puts(0, (STRPTR)", access: "); _aio_puts2(0, access);
    _aio_puts(0, (STRPTR)", fno: "); _aio_puts2(0, fno);
    _aio_puts(0, (STRPTR)", recordlen: "); _aio_puts2(0, recordlen);
    _aio_putnl(0);
#endif
    if ( !fno || (fno >=AIO_MAX_FILES) || g_files[fno])
    {
        ERROR(AE_OPEN);
        return;
    }

    int am = (mode == FILE_MODE_OUTPUT) ? MODE_NEWFILE : MODE_OLDFILE;
    g_files[fno] = BADDR(Open (fname, am));
    if (!g_files[fno])
    {
        ERROR(AE_OPEN);
        return;
    }

    if (mode == FILE_MODE_APPEND)
        Seek (MKBADDR(g_files[fno]), 0, OFFSET_END);
}

void _aio_fputs(USHORT fno, const UBYTE *s)
{
    if ( !fno || (fno >=AIO_MAX_FILES) || !g_files[fno])
    {
        ERROR(AE_OUTPUT);
        return;
    }
    ULONG l = LEN_(s);
    Write(MKBADDR(g_files[fno]), (CONST APTR) s, l);
}

struct FileHandle *_aio_getfh (USHORT fno)
{
    if ( !fno || (fno >=AIO_MAX_FILES) || !g_files[fno])
    {
        ERROR(AE_OUTPUT);
        return NULL;
    }
    return g_files[fno];
}

void _aio_init(void)
{
    for (UWORD i=0; i<AIO_MAX_FILES; i++)
        g_files[i] = NULL;
}

void _aio_close (USHORT fno)
{
    if (!fno)
    {
        for (UWORD i=1; i<AIO_MAX_FILES; i++)
        {
            if (!g_files[i])
                continue;

            Close(MKBADDR(g_files[i]));
            g_files[i] = NULL;
        }
    }
    else
    {
        if ( (fno >=AIO_MAX_FILES) || !g_files[fno] )
        {
            ERROR(AE_CLOSE);
            return;
        }
        Close(MKBADDR(g_files[fno]));
        g_files[fno] = NULL;
    }
}

void _aio_shutdown(void)
{
    for (UWORD i=0; i<AIO_MAX_FILES; i++)
    {
        if (g_files[i])
            Close (MKBADDR(g_files[i]));
    }
}

