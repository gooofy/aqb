#include "terminal.h"

#include <stdlib.h>
#include <stdarg.h>

#ifdef __amigaos__

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <devices/conunit.h>

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

static BPTR                  g_raw_out;
static BPTR                  g_raw_in;

static struct Window        *g_con_win;
static struct ConUnit       *g_con_unit;

#define CSI       "\x9b"

#else

#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#define CSI       "\033["

#endif


#define BUFSIZE   2048
static char            g_outbuf[BUFSIZE];
static int             g_bpos = 0;
static void            (*g_size_cb)(void) = NULL;


#ifdef __amigaos__

static long doDosPacket (struct MsgPort *pid, long action, long *args, long nargs)
{
    struct MsgPort        *replyport;
    struct StandardPacket *packet;
    long                   count, *pargs, res1;

    replyport = (struct MsgPort *) CreatePort (NULL, 0);
    if (!replyport)
        return (0);

    packet = (struct StandardPacket *) AllocMem ((long) sizeof(struct StandardPacket), MEMF_PUBLIC | MEMF_CLEAR);
    if (!packet)
    {
        DeletePort(replyport);
        return 0;
    }

    packet->sp_Msg.mn_Node.ln_Name = (char *) &(packet->sp_Pkt);
    packet->sp_Pkt.dp_Link = &(packet->sp_Msg);
    packet->sp_Pkt.dp_Port = replyport;
    packet->sp_Pkt.dp_Type = action;

    pargs = &(packet->sp_Pkt.dp_Arg1);
    for (count = 0; count < nargs; count++)
        pargs[count] = args[count];

    PutMsg(pid, (struct Message *)packet);

    WaitPort(replyport);
    GetMsg(replyport);

    res1 = packet->sp_Pkt.dp_Res1;

    FreeMem (packet, (long) sizeof(struct StandardPacket));
    DeletePort (replyport);

    return res1;
}

static bool get_ConUnit(void)
{
    struct MsgPort  *mp;
    struct InfoData *id;
    long             arg, res;

    mp = ((struct FileHandle *) (BADDR(g_raw_in)))->fh_Type;

    id = (struct InfoData *) AllocMem (sizeof(struct InfoData), MEMF_PUBLIC | MEMF_CLEAR);
    if (!id)
        return FALSE;

    arg = ((ULONG) id) >> 2;
    res = doDosPacket (mp, ACTION_DISK_INFO, &arg, 1);

    g_con_win = (struct Window *) id->id_VolumeNode;
    g_con_unit  = (struct ConUnit *) ((struct IOStdReq *) id->id_InUse)->io_Unit;

    FreeMem (id, sizeof(struct InfoData));

    return res != 0;
}

static long changeScreenMode (bool rawMode)
{
    struct MsgPort *mp;
    long            arg;

    mp = ((struct FileHandle *) (BADDR(g_raw_in)))->fh_Type;
    arg = rawMode ? -1 : 0;
    return doDosPacket(mp, ACTION_SCREEN_MODE, &arg, 1);
}

void TE_flush(void)
{
    if (g_bpos != 0)
        Write(g_raw_out, g_outbuf, g_bpos);
    g_bpos = 0;
}

// FIXME: implement size change callback

bool TE_getsize(int *rows, int *cols)
{
    if (!get_ConUnit())
        return FALSE;

    *rows = g_con_unit->cu_YMax + 1;
    *cols = g_con_unit->cu_XMax + 1;

    /* range checks */

    if ( (*rows < 0) || (*rows > 256) )
    {
        *rows = 80;
        *rows = 24;
    }

    if (*cols < TE_MIN_COLUMNS)
        *cols = TE_MIN_COLUMNS;
    if (*cols > TE_MAX_COLUMNS)
        *cols = TE_MAX_COLUMNS;

    if (*rows < TE_MIN_ROWS)
        *rows = TE_MIN_ROWS;
    if (*rows > TE_MAX_ROWS)
        *rows = TE_MAX_ROWS;

    return TRUE;
}

static void TE_exit(void)
{
    TE_setTextStyle (TE_STYLE_NORMAL);
    TE_putstr(CSI "12}"); /* window resize events de-activated */
    TE_flush();

    if (g_raw_in != g_raw_out)
    {
        if (!changeScreenMode(/*raw_mode=*/FALSE))
            fprintf(stderr, "AQB: Can't change to cooked mode\n");
    }
    else
    {
        Close(g_raw_in);
    }
}

bool TE_init (void)
{
    g_raw_in = Input();
    if (!IsInteractive(g_raw_in))
    {
        g_raw_in = Open((STRPTR) "RAW:0/0/640/200/AQB", MODE_NEWFILE);
        if (!g_raw_in)
        {
            fprintf(stderr, "AQB: Can't open window\n");
            return FALSE;
        }
        g_raw_out = g_raw_in;
    }
    else
    {
        g_raw_out = Output();
        if (!changeScreenMode(/*raw_mode=*/TRUE))
        {
            fprintf(stderr, "AQB: Can't change to raw mode");
            return FALSE;
        }
    }

    TE_putstr(CSI "12{"); /* window resize events activated */
    TE_flush();

	atexit(TE_exit);

	return TRUE;
}

int TE_getch(void)
{
    char            c;
    Read(g_raw_in, &c, sizeof(c));
    return (int) c;
}

#else

void TE_flush  (void)
{
    if (g_bpos != 0)
        write(STDOUT_FILENO, g_outbuf, g_bpos);
    g_bpos = 0;
}

int TE_getch (void)
{
    int nread;
    char c, seq[3];
    while ((nread = read(STDIN_FILENO,&c,1)) == 0);
    if (nread == -1)
        exit(1);

    while(1)
    {
        if (c==KEY_ESC) // handle escape sequences
        {
            /* If this is just an ESC, we'll timeout here. */
            if (read(STDIN_FILENO,seq,1) == 0)
                return KEY_ESC;
            if (read(STDIN_FILENO,seq+1,1) == 0)
                return KEY_ESC;

            /* ESC [ sequences. */
            if (seq[0] == '[')
            {
                if (seq[1] >= '0' && seq[1] <= '9')
                {
                    /* Extended escape, read additional byte. */
                    if (read(STDIN_FILENO,seq+2,1) == 0)
                        return KEY_ESC;
                    if (seq[2] == '~')
                    {
                        switch(seq[1])
                        {
                            case '3': return KEY_DEL;
                            case '5': return KEY_PAGE_UP;
                            case '6': return KEY_PAGE_DOWN;
                        }
                    }
                }
                else
                {
                    switch(seq[1])
                    {
                        case 'A': return KEY_CURSOR_UP;
                        case 'B': return KEY_CURSOR_DOWN;
                        case 'C': return KEY_CURSOR_RIGHT;
                        case 'D': return KEY_CURSOR_LEFT;
                        case 'H': return KEY_HOME;
                        case 'F': return KEY_END;
                    }
                }
            }

            /* ESC O sequences. */
            else if (seq[0] == 'O')
            {
                switch(seq[1])
                {
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        }
        else
        {
            return c;
        }
    }
    return 0;
}

static void handleSigWinCh(int unused __attribute__((unused)))
{
    if (g_size_cb)
        g_size_cb();
}

struct termios g_orig_termios;
static void TE_exit (void)
{
    TE_setTextStyle (TE_STYLE_NORMAL);
	// disable raw mode
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &g_orig_termios);
}

bool TE_init (void)
{
    signal(SIGWINCH, handleSigWinCh);

	// enable raw mode
    struct termios raw;

    if (!isatty(STDIN_FILENO))
		goto fatal;
    atexit(TE_exit);
    if (tcgetattr(STDOUT_FILENO, &g_orig_termios) == -1)
		goto fatal;

    raw = g_orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &raw) < 0)
		goto fatal;
    return TRUE;

fatal:
    errno = ENOTTY;
    return FALSE;
}

static bool getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return FALSE;

    while (i < sizeof(buf)-1)
	{
        if (read(STDIN_FILENO,buf+i,1) != 1)
			break;
        if (buf[i] == 'R')
			break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != KEY_ESC || buf[1] != '[')
		return FALSE;
    if (sscanf(buf+2,"%d;%d",rows,cols) != 2)
		return FALSE;
    return TRUE;
}


bool TE_getsize(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
        int orig_row, orig_col;

        if (!getCursorPosition(&orig_row, &orig_col))
			goto failed;

        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			goto failed;
        if (!getCursorPosition(rows, cols))
			goto failed;

        char seq[32];
        snprintf(seq, 32, "\x1b[%d;%dH", orig_row, orig_col);
        write(STDOUT_FILENO, seq, strlen(seq));
    }
	else
	{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
    }
	return TRUE;

failed:
    return FALSE;
}

#endif

void TE_putc(char c)
{
    g_outbuf[g_bpos++] = c;
    if (g_bpos >= BUFSIZE)
        TE_flush();
}

void TE_putstr(char *s)
{
    while (*s)
    {
        g_outbuf[g_bpos++] = *s++;
        if (g_bpos >= BUFSIZE)
            TE_flush();
    }
}

void TE_printf (char* format, ...)
{
    va_list args;
    char buf[BUFSIZE];
    va_start(args, format);
    vsnprintf (buf, BUFSIZE, format, args);
    va_end(args);
    TE_putstr(buf);
}

void TE_onSizeChangeCall (void (*cb)(void))
{
    g_size_cb = cb;
}

void TE_moveCursor (int row, int col)
{
    TE_printf (CSI "%d;%d;H", row, col);
}

void TE_eraseToEOL (void)
{
    TE_putstr (CSI "K");
}

void TE_eraseDisplay (void)
{
    TE_putstr (CSI "J");
}

void TE_bell (void)
{
    TE_putstr ("\007");
}

void TE_setCursorVisible (bool visible)
{
#ifdef __amiga__
    if (visible)
        TE_putstr ( CSI " p");
    else
        TE_putstr ( CSI "0 p");
#else
    if (visible)
        TE_putstr ( CSI "?25h");
    else
        TE_putstr ( CSI "?25l");
#endif
}

void TE_setTextStyle (int style)
{
    TE_printf ( CSI "%dm", style);
}

float TE_get_time (void)
{
    #ifdef __amigaos__
        struct DateStamp datetime;
        DateStamp(&datetime);
        return datetime.ds_Minute * 60.0 + datetime.ds_Tick / 50.0;

    #else

        clock_t t = clock();
        return ((float)t)/CLOCKS_PER_SEC;

    #endif
}


