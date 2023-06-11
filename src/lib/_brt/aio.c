//#define ENABLE_DPRINTF

#include "_brt.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

#define MAXBUF 			  40
#define MAX_INPUT_BUF   1024
#define MAX_TOKEN_LEN   1024
#define MAX_LINE_LEN    1024

#define AIO_MAX_FILES 	  16

#define CSI 			0x9b

typedef struct
{
    struct FileHandle *fh;
    BPTR               fhBPTR;
    UBYTE              input_buffer[MAX_INPUT_BUF];
    LONG               input_pos;
    LONG               input_len;
    BOOL               eof;
    BOOL               getch_done;
    UBYTE              input_ch;
    UBYTE              input_token[MAX_TOKEN_LEN+1]; // add room for final \0
} fileinfo_t;

static fileinfo_t *g_fis[AIO_MAX_FILES];

_aio_puts_cb_t   _aio_puts_cb   = NULL;
_aio_gets_cb_t   _aio_gets_cb   = NULL;
_aio_cls_cb_t    _aio_cls_cb    = NULL;
_aio_locate_cb_t _aio_locate_cb = NULL;

static BPTR g_stdout, g_stdin;

void _AIO_PUTS (USHORT fno, const UBYTE *s)
{
    if (fno)
    {
        DPRINTF ("_aio_puts: fno=%d -> file i/o\n", fno);
        if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
        {
            ERROR(ERR_BAD_FILE_NUMBER);
            return;
        }
        ULONG l = _astr_len(s);
        Write(g_fis[fno]->fhBPTR, (CONST APTR) s, l);
        return;
    }

    if (_aio_puts_cb && _aio_puts_cb ( (CONST STRPTR) s))
    {
        DPRINTF ("_aio_puts: fno=%d -> _aqb\n", fno);
        return;
    }

    DPRINTF ("_aio_puts: fno=%d -> CLI, s='%s'\n", fno, s);
    ULONG l = _astr_len(s);
    Write(g_stdout, (CONST APTR) s, l);
}

void _AIO_SET_DOS_CURSOR_VISIBLE (BOOL visible)
{
    static UBYTE csr_on[]   = { CSI, '1', ' ', 'p', '\0' };
    static UBYTE csr_off[]  = { CSI, '0', ' ', 'p', '\0' };

    UBYTE *c = visible ? csr_on : csr_off;
    Write(g_stdout, (CONST APTR) c, _astr_len(c));
}

void _AIO_PUTNL(USHORT fno)
{
    _AIO_PUTS(fno, (UBYTE*)"\n");
}

void _AIO_PUTTAB(USHORT fno)
{
    _AIO_PUTS(fno, (UBYTE*)"\t");
}

void _AIO_PUTS4(USHORT fno, LONG num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTS2(USHORT fno, short num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTS1(USHORT fno, BYTE num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 10);

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTU4(USHORT fno, ULONG num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa(num, buf, 10);

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTU2(USHORT fno, USHORT num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa(num, buf, 10);

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTU1(USHORT fno, UBYTE num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa(num, buf, 10);

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTHEX(USHORT fno, LONG num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 16);

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTBIN(USHORT fno, LONG num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa(num, buf, 2);

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTUHEX(USHORT fno, ULONG l)
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

    _AIO_PUTS(fno, buf);
}

void _AIO_PUTF(USHORT fno, FLOAT f)
{
    //DPRINTF ("_aio_putf: fno=%d\n", fno);
    //Delay(200);
    UBYTE buf[40];
    _astr_ftoa(f, buf);
    //DPRINTF ("_aio_putf: -> _aio_puts buf=%s\n", buf); Delay(200);
    _AIO_PUTS(fno, buf);
}

void _AIO_PUTBOOL(USHORT fno, BOOL b)
{
    DPRINTF("_AIO_PUTBOOL fno=%d, b=%d\n", fno, b);
    _AIO_PUTS(fno, b ? (UBYTE*)"TRUE" : (UBYTE*)"FALSE");
}

/*********************************************************
 *
 * WRITE support
 *
 *********************************************************/

void _AIO_WRITES4 (USHORT fno, LONG num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa_ext(num, buf, 10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);

    _AIO_PUTS(fno, buf);
}

void _AIO_WRITES2 (USHORT fno, short num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa_ext(num, buf, 10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);

    _AIO_PUTS(fno, buf);
}

void _AIO_WRITES1 (USHORT fno, BYTE num)
{
    UBYTE buf[MAXBUF];

    _astr_itoa_ext(num, buf, 10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);

    _AIO_PUTS(fno, buf);
}

void _AIO_WRITEU4 (USHORT fno, ULONG num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa_ext(num, buf, 10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);

    _AIO_PUTS(fno, buf);
}

void _AIO_WRITEU2 (USHORT fno, USHORT num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa_ext(num, buf, 10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);

    _AIO_PUTS(fno, buf);
}

void _AIO_WRITEU1 (USHORT fno, UBYTE num)
{
    UBYTE buf[MAXBUF];

    _astr_utoa_ext(num, buf, 10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);

    _AIO_PUTS(fno, buf);
}

void _AIO_WRITEF (USHORT fno, FLOAT f)
{
    //DPRINTF ("_aio_writef: fno=%d\n", fno);
    //Delay(200);
    UBYTE buf[40];
    _astr_ftoa_ext (f, buf, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);
    //DPRINTF ("_aio_writef: -> _aio_writes buf=%s\n", buf); Delay(200);
    _AIO_PUTS(fno, buf);
}

void _AIO_WRITEBOOL (USHORT fno, BOOL b)
{
    _AIO_PUTS(fno, b ? (UBYTE*)"TRUE" : (UBYTE*)"FALSE");
}

void _AIO_WRITES (USHORT fno, const UBYTE *str)
{
    _AIO_PUTS(fno, (STRPTR)"\"");
    _AIO_PUTS(fno, str);
    _AIO_PUTS(fno, (STRPTR)"\"");
}

void _AIO_WRITECOMMA (USHORT fno, BOOL b)
{
    _AIO_PUTS(fno, (STRPTR)",");
}

/*********************************************************
 *
 * [LINE] INPUT support
 *
 *********************************************************/

static inline BOOL _buffer (USHORT fno)
{
    //DPRINTF ("_buffer: fno=%d\n", fno);

    fileinfo_t *fi = g_fis[fno];

    if (fi->input_pos < fi->input_len)
        return TRUE;

    if (fi->eof)
    {
        //DPRINTF ("_buffer: fi->eof\n");
        return FALSE;
    }

    LONG n = Read (fi->fhBPTR, fi->input_buffer, MAX_INPUT_BUF);
    //DPRINTF ("_buffer: Read()->%ld\n", n);
    if (n<0)
    {
        fi->eof = TRUE;
        ERROR(ERR_IO_ERROR);
        return FALSE;
    }
    fi->input_pos = 0;
    fi->input_len = n;
    if (n==0)
    {
        fi->eof = TRUE;
        return FALSE;
    }

    return TRUE;
}

// returns TRUE if successful, FALSE otherwise
static BOOL _input_getch (USHORT fno)
{
    fileinfo_t *fi = g_fis[fno];

    if (!fno)
    {
        if (fi->input_pos >= fi->input_len)
            return FALSE;
        fi->input_ch = fi->input_buffer[fi->input_pos++];
        return TRUE;
    }

    if (!_buffer (fno))
        return FALSE;

    fi->input_ch = fi->input_buffer[fi->input_pos++];
    return TRUE;
}

static void _aio_gets (UBYTE *buf, USHORT buf_len, BOOL do_nl)
{
    if (_aio_gets_cb && _aio_gets_cb(buf, buf_len, do_nl) )
    {
        DPRINTF ("_aio_gets: _aqb\n");
        return;
    }

    _AIO_SET_DOS_CURSOR_VISIBLE (TRUE);
    LONG bytes = Read(g_stdin, (CONST APTR) buf, MAX_LINE_LEN);
    buf[bytes-1] = '\0';
    _AIO_SET_DOS_CURSOR_VISIBLE (FALSE);

    DPRINTF ("_aio_gets: buf=%s\n", buf);
}

void _AIO_LINE_INPUT (USHORT fno, UBYTE *prompt, UBYTE **s, BOOL do_nl)
{
    static UBYTE buf[MAX_LINE_LEN+1];

    DPRINTF ("_aio_line_input: fno=%d, prompt=%s, *s=0x%08lx, do_nl=%d\n", fno, prompt ? (char *) prompt : "NULL", *s, do_nl);

    if (fno)
    {
        DPRINTF ("_aio_line_input: fno=%d -> file i/o\n", fno);
        if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
        {
            ERROR(ERR_BAD_FILE_NUMBER);
            return;
        }

        int l = 0;

        while (l<MAX_LINE_LEN)
        {
            BOOL res = _input_getch (fno);
            UBYTE c = g_fis[fno]->input_ch;
            if (!res || (c=='\r') || (c=='\n'))
                break;
            buf[l++] = c;
        }
        buf[l] = 0;

        *s = _astr_dup (buf);
        return;
    }

    if (prompt)
        _AIO_PUTS(/*fno=*/0, prompt);

    _aio_gets (buf, MAX_LINE_LEN, do_nl);

    *s = _astr_dup (buf);
}

#if 0
static BOOL _is_whitespace (char c)
{
    return (c == ' ') || (c == '\t');
}

static BOOL _input_skip_whitespace (USHORT fno)
{
    fileinfo_t *fi = g_fis[fno];
    BOOL eof = FALSE;
    while (_is_whitespace (fi->input_ch) && !eof)
        eof = !_input_getch (fno);
    return eof;
}

static void _input_skip_delimiter (USHORT fno)
{
    BOOL eof = _input_skip_whitespace(fno);
    fileinfo_t *fi = g_fis[fno];

    if (eof)
        return;

    BOOL delim_found = FALSE;
    while (!delim_found && !eof)
    {
        DPRINTF("_input_skip_delimiter: skipping... (fi_input_ch=%c[%d])\n", fi->input_ch, fi->input_ch);
        switch (fi->input_ch)
        {
            case ',':
            case '\n':
                if (!_input_getch(fno))
                    eof = TRUE;
                delim_found = TRUE;
                break;

            case '\r':
                if (!_input_getch(fno))
                    eof = TRUE;
                if (!eof && fi->input_ch == '\n')
                {
                    if (!_input_getch(fno))
                        eof = TRUE;
                    delim_found = TRUE;
                }
                break;
            default:
                if (!_input_getch(fno))
                    eof = TRUE;
                break;
        }
    }
}

static void _input_next_token (USHORT fno)
{
    fileinfo_t *fi  = g_fis[fno];
    BOOL        eof = FALSE;

    if (!fi->getch_done)
    {
        if (!_input_getch(fno))
            eof = TRUE;
        fi->getch_done = TRUE;
    }
    DPRINTF("_input_next_token: getch_done=%d, input_ch=%d\n", fi->getch_done, fi->input_ch);

    DPRINTF("_input_next_token: skip whitespace...\n");
    if (_input_skip_whitespace(fno))
        ERROR (ERR_OUT_OF_DATA); // FIXME: in console input, re-prompt the user

    BOOL        skip_delim = TRUE;
    BOOL        in_quote   = FALSE;
    int         len        = 0;

    while (!eof && (len < MAX_TOKEN_LEN))
    {
        DPRINTF("_input_next_token: token... (fi_input_ch=%c[%d])\n", fi->input_ch, fi->input_ch);
        switch (fi->input_ch)
        {
            case '\n':
                if (!_input_getch(fno))
                    eof = TRUE;
                skip_delim = FALSE;
                goto fini;

            case '\r':
                if (!_input_getch(fno))
                    eof = TRUE;
                if (!eof && (fi->input_ch == '\n') )
                {
                    if (!_input_getch(fno))
                        eof = TRUE;
                    skip_delim = FALSE;
                    goto fini;
                }
                break;

            case '"':
                if (in_quote)
                {
                    if (!_input_getch(fno))
                        eof = TRUE;
                    goto fini;
                }

                if (len == 0)
                {
                    in_quote = TRUE;
                    if (!_input_getch(fno))
                        eof = TRUE;
                    continue;
                }
                break;

            case ',':
                if (!in_quote)
                {
                    if (!_input_getch(fno))
                        eof = TRUE;
                    skip_delim = FALSE;
                    goto fini;
                }
                break;

            case '\t':
            case ' ':
                if (!in_quote)
                {
                    if (!_input_getch(fno))
                        eof = TRUE;
                    if (fno)
                    {
                        skip_delim = FALSE;
                        goto fini;
                    }
                    continue;
                }
                break;

            default:
                break;
        }

        fi->input_token[len++] = fi->input_ch;
        if (!_input_getch(fno))
            eof = TRUE;
    }

fini:
    fi->input_token[len++] = '\0';
    DPRINTF("_input_next_token: fini fi->input_token=%s\n", fi->input_token);

    // DPRINTF("delim\n");
    if (skip_delim)
        _input_skip_delimiter (fno);
}
#endif

void _AIO_CONSOLE_INPUT (BOOL qm, UBYTE *prompt, BOOL do_nl)
{
    DPRINTF ("aio: _aio_console_input qm=%d, prompt=%s, do_nl=%d\n", qm, prompt ? (char *)prompt : "NULL", do_nl);

    if (prompt)
        _AIO_PUTS (/*fno=*/0, prompt);
    if (qm)
        _AIO_PUTS (/*fno=*/0, (STRPTR)"?");

    fileinfo_t *fi = g_fis[0];

    if (!fi)
    {
        fi = AllocVec(sizeof(*fi), MEMF_CLEAR);
        if (!fi)
        {
            DPRINTF ("_aio_console_input: out of memory\n");
            ERROR(ERR_OUT_OF_MEMORY);
            return;
        }

        fi->fhBPTR     = 0;
        fi->fh         = NULL;
        fi->input_pos  = 0;
        fi->input_len  = 0;
        fi->eof        = FALSE;
        fi->getch_done = FALSE;

        g_fis[0] = fi;
    }

    _aio_gets (fi->input_buffer, MAX_INPUT_BUF, do_nl);

    fi->input_pos = 0;
    fi->input_len = _astr_len(fi->input_buffer);
    fi->eof       = FALSE;

    //_input_getch(/*fno=*/0);
}

#if 0

void _AIO_INPUTS1 (USHORT fno, BYTE   *v)
{
    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }
    _input_next_token (fno);
    *v = VALINT_ (g_fis[fno]->input_token);
}

void _AIO_INPUTU1 (USHORT fno, UBYTE  *v)
{
    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }
    _input_next_token (fno);
    *v = VALINT_ (g_fis[fno]->input_token);
}

void _AIO_INPUTS2 (USHORT fno, SHORT  *v)
{
    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }
    _input_next_token (fno);
    *v = VALINT_ (g_fis[fno]->input_token);
}

void _AIO_INPUTU2 (USHORT fno, USHORT *v)
{
    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }
    _input_next_token (fno);
    *v = VALUINT_ (g_fis[fno]->input_token);
}

void _AIO_INPUTS4 (USHORT fno, LONG   *v)
{
    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }
    _input_next_token (fno);
    *v = VALLNG_ (g_fis[fno]->input_token);
}

void _AIO_INPUTU4 (USHORT fno, ULONG  *v)
{
    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }
    _input_next_token (fno);
    *v = VALULNG_ (g_fis[fno]->input_token);
}

void _AIO_INPUTF  (USHORT fno, FLOAT  *v)
{
    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }
    _input_next_token (fno);
    DPRINTF ("_AIO_INPUTF: token=%s\n", g_fis[fno]->input_token);
    *v = VAL_ (g_fis[fno]->input_token);
}

void _AIO_INPUTS (USHORT fno, UBYTE **v)
{
    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }
    _input_next_token (fno);
    *v = _astr_dup(g_fis[fno]->input_token);
}
#endif

void _AIO_OPEN (UBYTE *fname, USHORT mode, USHORT access, USHORT fno, USHORT recordlen)
{
    DPRINTF ("_aio_open: fname=%s, mode=%d, access=%d, fno=%d, rl=%d\n",
             fname, mode, access, fno, recordlen);

    if ( !fno || (fno >=AIO_MAX_FILES) || g_fis[fno])
    {
        DPRINTF ("_aio_open: invalid arguments\n");
        ERROR(ERR_BAD_FILE_NUMBER);
        return;
    }

    // FIXME: ensure fi is freed on exit!
    fileinfo_t *fi = AllocVec(sizeof(*fi), MEMF_CLEAR);
    if (!fi)
    {
        DPRINTF ("_aio_open: out of memory\n");
        ERROR(ERR_OUT_OF_MEMORY);
        return;
    }

    int am = (mode == FILE_MODE_OUTPUT) ? MODE_NEWFILE : MODE_OLDFILE;
    fi->fhBPTR = Open (fname, am);
    if (!fi->fhBPTR)
    {
        FreeVec (fi);
        DPRINTF ("_aio_open: Open() failed\n");
        ERROR(ERR_BAD_FILE_NAME);
        return;
    }

    if (mode == FILE_MODE_APPEND)
        Seek (fi->fhBPTR, 0, OFFSET_END);

    fi->fh = BADDR(fi->fhBPTR);
    fi->input_pos  = 0;
    fi->input_len  = 0;
    fi->eof        = FALSE;
    fi->getch_done = FALSE;

    g_fis[fno] = fi;
}

struct FileHandle *_aio_getfh (USHORT fno)
{
    if ( !fno || (fno >=AIO_MAX_FILES) || !g_fis[fno])
    {
        ERROR(ERR_BAD_FILE_NUMBER);
        return NULL;
    }
    return g_fis[fno]->fh;
}

void _aio_init(void)
{
    g_stdout = Output();
    g_stdin  = Input();

	_AIO_SET_DOS_CURSOR_VISIBLE (FALSE);

    for (UWORD i=0; i<AIO_MAX_FILES; i++)
        g_fis[i] = NULL;
}

static void _close (USHORT fno)
{
    Close (g_fis[fno]->fhBPTR);
    FreeVec (g_fis[fno]);
    g_fis[fno] = NULL;
}

void _AIO_CLOSE (USHORT fno)
{
    DPRINTF ("_aio_close: fno=%d\n", fno);
    if (!fno)
    {
        for (UWORD i=1; i<AIO_MAX_FILES; i++)
        {
            if (!g_fis[i])
                continue;
            _close (i);
        }
    }
    else
    {
        if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
        {
            DPRINTF ("_aio_close: ***ERRROR: invalid fno / file not open\n");
            ERROR(ERR_BAD_FILE_NUMBER);
            return;
        }
        _close (fno);
    }
}

BOOL EOF_ (USHORT fno)
{
    DPRINTF ("EOF_: fno=%d\n", fno);

    if ( (fno >=AIO_MAX_FILES) || !g_fis[fno] )
    {
        DPRINTF ("EOF_: ***ERRROR: invalid fno / file not open\n");
        ERROR(ERR_BAD_FILE_NUMBER);
        return TRUE;
    }

    return !_buffer (fno);
}

void CLS (void)
{
    if (_aio_cls_cb && _aio_cls_cb() )
    {
        DPRINTF ("CLS: _aqb\n");
        return;
    }

    char form_feed = 0x0c;
    Write(g_stdout, (CONST APTR) &form_feed, 1);
    return;
}

void LOCATE (SHORT line, SHORT col)
{
    if (_aio_locate_cb && _aio_locate_cb(line, col) )
    {
        DPRINTF ("LOCATE: _aqb\n");
        return;
    }

    UBYTE buf[20];
    buf[0] = CSI;
    _astr_itoa_ext(line, &buf[1], 10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);

    int l = _astr_len(buf);
    buf[l] = ';';
    l++;
    _astr_itoa_ext(col, &buf[l], 10, /*leading_space=*/FALSE, /*positive_sign=*/FALSE);
    l = _astr_len(buf);
    buf[l] = 'H';
    buf[l+1] = 0;

    Write(g_stdout, (CONST APTR) buf, l+1);
    return;
}


void _aio_shutdown(void)
{
    for (UWORD i=0; i<AIO_MAX_FILES; i++)
    {
        if (g_fis[i])
            Close (g_fis[i]->fhBPTR);
    }

	_AIO_SET_DOS_CURSOR_VISIBLE (TRUE);
}

