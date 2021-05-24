#ifdef __amigaos__

#include "ui.h"

#include <stdlib.h>
#include <stdarg.h>
#include "logger.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <devices/conunit.h>

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#include <proto/console.h>
#include <pragmas/console_pragmas.h>

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
//#include <clib/mathffp_protos.h>
#include <clib/console_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/intuition.h>
#include <inline/graphics.h>
//#include <inline/mathffp.h>

#include <libraries/reqtools.h>
#include <inline/reqtools.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;
extern struct GfxBase       *GfxBase;
extern struct IntuitionBase *IntuitionBase;
struct ReqToolsBase         *ReqToolsBase;

#define CSI       "\x9b"

#define BUFSIZE   2048
static char            g_outbuf[BUFSIZE];
static int             g_bpos = 0;
static UI_size_cb      g_size_cb = NULL;
static void           *g_size_cb_user_data = NULL;

static UI_key_cb       g_key_cb = NULL;
static void           *g_key_cb_user_data = NULL;
static uint16_t        g_scrollStart   = 0;
static uint16_t        g_scrollEnd     = 10;

void __request (string msg);

#define NEWLIST(l) ((l)->lh_Head = (struct Node *)&(l)->lh_Tail, \
                    /*(l)->lh_Tail = NULL,*/ \
                    (l)->lh_TailPred = (struct Node *)&(l)->lh_Head)

static struct NewWindow g_nw =
{
    0, 0, 640, 200,
    -1,-1,                            /* detailpen, blockpen */
    CLOSEWINDOW,                      /* IDCMP */
    WINDOWDEPTH|WINDOWSIZING|
    WINDOWDRAG|WINDOWCLOSE|
    SMART_REFRESH|ACTIVATE,           /* window flags */
    NULL, NULL,
    (uint8_t *) "AQB",
    NULL,
    NULL,
    100,45,                           /* min width, height */
    1024, 768,                        /* max width, height */
    WBENCHSCREEN
};

static struct Window     *g_win           = NULL;
static struct IOStdReq   *g_writeReq      = NULL;
static struct MsgPort    *g_writePort     = NULL;
static struct IOStdReq   *g_readReq       = NULL;
static struct MsgPort    *g_readPort      = NULL;
static bool               g_ConOpened     = FALSE;
static struct FileHandle *g_output        = NULL;
static struct MsgPort    *g_IOport        = NULL;
static int                g_termSignalBit = 0;


static struct MsgPort *create_port(STRPTR name, LONG pri)
{
    struct MsgPort *port = NULL;
    UBYTE portsig;

    if ((BYTE)(portsig=AllocSignal(-1)) >= 0)
    {
        if (!(port=AllocMem(sizeof(*port),MEMF_CLEAR|MEMF_PUBLIC)))
        {
            FreeSignal(portsig);
        }
        else
        {
            port->mp_Node.ln_Type = NT_MSGPORT;
            port->mp_Node.ln_Pri  = pri;
            port->mp_Node.ln_Name = (char *)name;
            /* done via AllocMem
            port->mp_Flags        = PA_SIGNAL;
            */
            port->mp_SigBit       = portsig;
            port->mp_SigTask      = FindTask(NULL);
            NEWLIST(&port->mp_MsgList);
            if (port->mp_Node.ln_Name)
                AddPort(port);
        }
    }
    return port;
}
static void delete_port(struct MsgPort *port)
{
    if (port->mp_Node.ln_Name)
        RemPort(port);
    FreeSignal(port->mp_SigBit);
    FreeMem(port,sizeof(*port));
}
static struct IORequest *create_ext_io(struct MsgPort *port,LONG iosize)
{
    struct IORequest *ioreq = NULL;

    if (port && (ioreq=AllocMem(iosize,MEMF_CLEAR|MEMF_PUBLIC)))
    {
        ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        ioreq->io_Message.mn_ReplyPort    = port;
        ioreq->io_Message.mn_Length       = iosize;
    }
    return ioreq;
}
static void delete_ext_io(struct IORequest *ioreq)
{
    LONG i;

    i = -1;
    ioreq->io_Message.mn_Node.ln_Type = i;
    ioreq->io_Device                  = (struct Device *)i;
    ioreq->io_Unit                    = (struct Unit *)i;
    FreeMem(ioreq,ioreq->io_Message.mn_Length);
}
static void queue_read(struct IOStdReq *readreq, UBYTE *whereto)
{
   readreq->io_Command = CMD_READ;
   readreq->io_Data = (APTR)whereto;
   readreq->io_Length = 1;
   SendIO((struct IORequest *)readreq);
}
static LONG con_may_get_char(struct MsgPort *msgport, UBYTE *whereto)
{
    struct IOStdReq *readreq;

    if (!(readreq = (struct IOStdReq *) GetMsg(msgport)))
		return -1;
    LONG temp = *whereto;                /* get the character */
    queue_read(readreq, whereto);     /* then re-use the request block */
    return temp;
}
#define MAX_ALERT_BUF 1024
static void cleanexit (char *s, uint32_t n)
{
    if (s)
    {
        if (IntuitionBase)
        {

            static char alertBuf[MAX_ALERT_BUF];
            snprintf (alertBuf, MAX_ALERT_BUF, "xxx%sxxxxPRESS LEFT MOUSE BUTTON TO CONTINUExx", s);

            int alertlen = strlen (alertBuf);
            int l = strlen(s);
            alertBuf[0]=0; alertBuf[1]=0x80; alertBuf[2]=0x14; // first line coords
            alertBuf[2+l]   = 0; // first line terminator
            alertBuf[2+l+1] = 1; // continuation byte
            alertBuf[2+l+2]=0; alertBuf[2+l+3]=0x80; alertBuf[2+l+4]=0x24; // second line coords
            alertBuf[alertlen-2]=0; // termination

            DisplayAlert(RECOVERY_ALERT, (STRPTR) alertBuf, alertlen);
        }
        printf(s);
    }
    exit(n);
}
static BYTE OpenConsole(void)
{
    BYTE error;

    g_writeReq->io_Data = (APTR) g_win;
    g_writeReq->io_Length = sizeof(struct Window);

    error = OpenDevice((uint8_t*)"console.device", 0, (struct IORequest*) g_writeReq, 0);

    g_readReq->io_Device = g_writeReq->io_Device;
    g_readReq->io_Unit   = g_writeReq->io_Unit;

    return error;
}
void UI_flush(void)
{
    if (g_bpos != 0)
    {
        g_writeReq->io_Command = CMD_WRITE;
        g_writeReq->io_Data    = (APTR) g_outbuf;
        g_writeReq->io_Length  = g_bpos;

        DoIO((struct IORequest *) g_writeReq);

        g_bpos = 0;
    }
}

// FIXME: implement size change callback

bool UI_getsize(uint16_t *rows, uint16_t *cols)
{
    struct ConUnit *g_con_unit = (struct ConUnit *) g_writeReq->io_Unit;
    *rows = g_con_unit->cu_YMax + 1;
    *cols = g_con_unit->cu_XMax + 1;

    /* range checks */

    if ( (*rows < 0) || (*rows > 256) )
    {
        *rows = 80;
        *rows = 24;
    }

    if (*cols < UI_MIN_COLUMNS)
        *cols = UI_MIN_COLUMNS;
    if (*cols > UI_MAX_COLUMNS)
        *cols = UI_MAX_COLUMNS;

    if (*rows < UI_MIN_ROWS)
        *rows = UI_MIN_ROWS;
    if (*rows > UI_MAX_ROWS)
        *rows = UI_MAX_ROWS;

    return TRUE;
}

struct FileHandle *UI_output (void)
{
    return g_output;
}

int UI_termSignal (void)
{
    return g_termSignalBit;
}

static void returnpacket(struct DosPacket *packet, long res1, long res2)
{
    struct Message *msg;
    struct MsgPort *replyport;

    packet->dp_Res1  = res1;
    packet->dp_Res2  = res2;
    replyport = packet->dp_Port;
    msg = packet->dp_Link;
    packet->dp_Port = g_IOport;
    msg->mn_Node.ln_Name = (char *)packet;
    msg->mn_Node.ln_Succ = NULL;
    msg->mn_Node.ln_Pred = NULL;

    PutMsg(replyport, msg);
}

static struct DosPacket *getpacket(void)
{
    struct Message *msg;
    msg = GetMsg(g_IOport);
    return ((struct DosPacket *)msg->mn_Node.ln_Name);
}


void UI_runIO (void)
{
    ULONG iosig   = 1 << g_IOport->mp_SigBit;
	ULONG termsig = 1 << g_termSignalBit;

    BOOL running = TRUE;
    while (running)
    {
        //LOG_printf (LOG_DEBUG, "term: UI_runIO: waiting for signal bits %d, %d ...\n", g_IOport->mp_SigBit, g_termSignalBit);
        ULONG signals = Wait(iosig | termsig);
        //LOG_printf (LOG_DEBUG, "term: UI_runIO: got signals: 0x%08x\n", signals);

        if (signals & iosig)
		{
			struct DosPacket *packet = getpacket();
			LOG_printf (LOG_DEBUG, "term: UI_runIO: got pkg, type=%d\n", packet->dp_Type);

			switch (packet->dp_Type)
			{
				case ACTION_WRITE:
				{
					LONG l = packet->dp_Arg3;
					char *buf = (char *)packet->dp_Arg2;
					//LOG_printf (LOG_DEBUG, "term: UI_runIO: ACTION_WRITE, len=%d\n", l);
					for (int i = 0; i<l; i++)
					{
                        char c = buf[i];
                        if (c=='\n')
                        {
                            uint16_t rows, cols;
                            UI_getsize(&rows, &cols);
                            UI_scrollUp (/*fullscreen=*/TRUE);
                            UI_moveCursor (rows+1, 1);
                            UI_eraseToEOL ();
                        }
                        else
                        {
                            UI_putc(c);
                        }
					}
					UI_flush();

					returnpacket (packet, l, packet->dp_Res2);
					break;
				}
				default:
					//LOG_printf (LOG_DEBUG, "term: UI_runIO: rejecting unknown packet type\n");
					returnpacket (packet, FALSE, ERROR_ACTION_NOT_KNOWN);
			}
		}
        else
        {
            if (signals & termsig)
            {
                running = FALSE;
            }
        }
	}
}

void UI_deinit(void)
{
    UI_flush();

    /* We always have an outstanding queued read request
     * so we must abort it if it hasn't completed,
     * and we must remove it.
     */
    if(!(CheckIO((struct IORequest *)g_readReq)))
		AbortIO((struct IORequest *)g_readReq);
    WaitIO((struct IORequest *)g_readReq);     /* clear it from our replyport */

    if (g_ConOpened)
        CloseDevice((struct IORequest *)g_writeReq);
    if (g_win)
        CloseWindow(g_win);
    if (g_readReq)
        delete_ext_io((struct IORequest *)g_readReq);
    if (g_readPort)
        delete_port(g_readPort);
    if (g_writeReq)
        delete_ext_io((struct IORequest *)g_writeReq);
    if (g_writePort)
        delete_port(g_writePort);
    if (ReqToolsBase)
        CloseLibrary((struct Library *)ReqToolsBase);
}

static UBYTE g_ibuf;

bool UI_init (void)
{
    SysBase = *(APTR *)4L;

    // check library versions

    if ( ((struct Library *)IntuitionBase)->lib_Version < 38)
    {
        __request ("intuition library is too old, need at least V38");
        exit(1);
    }
    if ( ((struct Library *)GfxBase)->lib_Version < 38)
    {
        __request ("graphics library is too old, need at least V38");
        exit(1);
    }
    if (!(ReqToolsBase = (struct ReqToolsBase *) OpenLibrary ((STRPTR)REQTOOLSNAME, REQTOOLSVERSION)))
         cleanexit("Can't open reqtools.library\n", RETURN_FAIL);
    if (!(g_writePort = create_port((STRPTR)"AQB.console.write",0)))
         cleanexit("Can't create write port\n", RETURN_FAIL);
    if (!(g_writeReq = (struct IOStdReq *) create_ext_io(g_writePort,(LONG)sizeof(struct IOStdReq))))
         cleanexit("Can't create write request\n", RETURN_FAIL);
    if(!(g_readPort = create_port((STRPTR)"AQB.console.read",0)))
         cleanexit("Can't create read port\n", RETURN_FAIL);
    if(!(g_readReq = (struct IOStdReq *) create_ext_io(g_readPort,(LONG)sizeof(struct IOStdReq))))
         cleanexit("Can't create read request\n", RETURN_FAIL);

    struct Screen *sc = LockPubScreen (NULL); // default public screen
    if (!sc)
         cleanexit("Failed to lock default public screen\n", RETURN_FAIL);

	// determine (visible) screen size and open full screen window on it

    ULONG mid = GetVPModeID(&sc->ViewPort);
    struct DimensionInfo di;
	if (!GetDisplayInfoData(NULL, (APTR)&di, sizeof(di), DTAG_DIMS, mid))
         cleanexit("Failed to retrieve display info data\n", RETURN_FAIL);

    //printf ("ui: workbench screen size is %d x %d (TxtOScan: (%d / %d) - (%d / %d)\n", (int)sc->Width, (int)sc->Height,
    //        (int)di.TxtOScan.MinX, (int)di.TxtOScan.MinY, (int)di.TxtOScan.MaxX, (int)di.TxtOScan.MaxY);

	WORD maxW = di.TxtOScan.MaxX - di.TxtOScan.MinX + 1;
	WORD maxH = di.TxtOScan.MaxY - di.TxtOScan.MinY + 1;

	g_nw.Width = sc->Width > maxW ? maxW : sc->Width;
	g_nw.Height = sc->Height > maxH ? maxH : sc->Height;

    if (!(g_win = OpenWindow(&g_nw)))
         cleanexit("Can't open window\n", RETURN_FAIL);

    UnlockPubScreen(NULL, sc);

    if (OpenConsole ())
         cleanexit("Can't open console.device\n", RETURN_FAIL);
    g_ConOpened = TRUE;

    UI_putstr(CSI "12{"); /* window resize events activated */
    UI_putstr(CSI ">1l"); /* auto scroll mode deactivated */
    UI_flush();

    queue_read(g_readReq, &g_ibuf); /* send the first console read request */

    /* prepare fake i/o filehandles (for IDE console redirection) */

	if ( !(g_output = AllocMem (sizeof(struct FileHandle), MEMF_CLEAR|MEMF_PUBLIC)) )
	{
		LOG_printf (LOG_ERROR, "run: failed to allocate memory for output file handle!\n");
		exit(24);
	}

	if (!(g_IOport = create_port ((STRPTR) "aqb_io_port", 0)))
	{
		LOG_printf (LOG_ERROR, "run: failed to create i/o port!\n");
		exit(25);
	}

	g_output->fh_Type = g_IOport;
	g_output->fh_Port = 0;
	g_output->fh_Args = (long)g_output;
	g_output->fh_Arg2 = (long)0;

	g_termSignalBit = AllocSignal(-1);
	if (g_termSignalBit == -1)
	{
		LOG_printf (LOG_ERROR, "run: failed to allocate signal bit!\n");
		exit(23);
	}

	return TRUE;
}

// handle CSI sequences: state machine

typedef enum {ESC_idle, ESC_esc1, ESC_csi, ESC_tilde, ESC_1,
              ESC_EVENT} ESC_state_t;

static inline void report_key (uint16_t key)
{
    if (g_key_cb)
        g_key_cb (key, g_key_cb_user_data);
}

static ESC_state_t g_esc_state = ESC_idle;
#define MAX_EVENT_BUF 32
static uint16_t nextKey(void)
{

/*
 * Window Bounds Report
 * Returned by the console device in response to a Window Status Request
 * sequence
 * 9B 31 3B 31 3B <bottom margin> 3B <right margin> 72
 *CSI  1  ;  1  ;                  ;                r
 */

    UBYTE ch;
    UBYTE event_buf[MAX_EVENT_BUF];
    uint16_t event_len=0;

    ULONG conreadsig = 1 << g_readPort->mp_SigBit;
    ULONG windowsig  = 1 << g_win->UserPort->mp_SigBit;

    BOOL running = TRUE;
    while (running)
    {
        ULONG signals = Wait(conreadsig|windowsig);

        if (signals & conreadsig)
		{
			LONG lch;
            if ((lch = con_may_get_char(g_readPort, &g_ibuf)) != -1)
			{
                ch = lch;
                LOG_printf (LOG_DEBUG, "terminal: *** got ch: 0x%02x, state=%d\n", ch, g_esc_state);
                switch (g_esc_state)
                {
                    case ESC_idle:
                        if ((ch==0x9b)||(ch==0x1b))
                            g_esc_state = ESC_esc1;
                        else
                            return ch;
                        break;
                    case ESC_esc1:
                        switch (ch)
                        {
                            case '[':
                                g_esc_state = ESC_csi;
                                break;
                            case 'A':
                                g_esc_state = ESC_idle;
                                return KEY_CURSOR_UP;
                                break;
                            case 'B':
                                g_esc_state = ESC_idle;
                                return KEY_CURSOR_DOWN;
                                break;
                            case 'C':
                                g_esc_state = ESC_idle;
                                return KEY_CURSOR_RIGHT;
                                break;
                            case 'D':
                                g_esc_state = ESC_idle;
                                return KEY_CURSOR_LEFT;
                                break;
                            case '0':
                                g_esc_state = ESC_tilde;
                                return KEY_F1;
                                break;
                            case '1':
                                g_esc_state = ESC_1;
                                break;
                            case '2':
                                g_esc_state = ESC_tilde;
                                return KEY_F3;
                                break;
                            case '3':
                                g_esc_state = ESC_tilde;
                                return KEY_F4;
                                break;
                            case '4':
                                g_esc_state = ESC_tilde;
                                return KEY_F5;
                                break;
                            case '5':
                                g_esc_state = ESC_tilde;
                                return KEY_F6;
                                break;
                            case '6':
                                g_esc_state = ESC_tilde;
                                return KEY_F7;
                                break;
                            case '7':
                                g_esc_state = ESC_tilde;
                                return KEY_F8;
                                break;
                            case '8':
                                g_esc_state = ESC_tilde;
                                return KEY_F9;
                                break;
                            case '9':
                                g_esc_state = ESC_tilde;
                                return KEY_F10;
                                break;
                            case '?':
                                g_esc_state = ESC_tilde;
                                return KEY_HELP;
                                break;
                            case 'T':
                                g_esc_state = ESC_idle;
                                return KEY_PAGE_UP;
                                break;
                            case 'S':
                                g_esc_state = ESC_idle;
                                return KEY_PAGE_DOWN;
                                break;
                            default:
                                g_esc_state = ESC_idle;
                                return KEY_UNKNOWN1;
                                break;
                        }
                        break;
                    case ESC_csi:
                        LOG_printf (LOG_DEBUG, "terminal: *** inside CSI sequence: 0x%02x\n", ch);
                        g_esc_state = ESC_idle;
                        break;
                    case ESC_tilde:
                        LOG_printf (LOG_DEBUG, "terminal: *** skipping tilde %c\n", ch);
                        g_esc_state = ESC_idle;
                        break;
                    case ESC_1:
                        /*
                         0x9b 0x31 0x32 0x3b 0x30 0x3b
                         CSI  1    2    ;    0    ;
                        */
                        LOG_printf (LOG_DEBUG, "terminal: ESC_1 %c\n", ch);
                        switch (ch)
                        {
                            case ';':
                            case '2':
                                LOG_printf (LOG_DEBUG, "terminal: EVENT stream detected\n", ch);
                                g_esc_state = ESC_EVENT;
                                event_len = 0;
                                break;
                            default:
                                return KEY_F2;
                        }
                        break;
                    case ESC_EVENT:
                        LOG_printf (LOG_DEBUG, "terminal: ESC_EVENT %c\n", ch);
                        if (ch == '|')
                        {
                            event_buf[event_len] = 0;
                            LOG_printf (LOG_DEBUG, "terminal: event report ended, buf: %s\n", event_buf);
                            if (g_size_cb)
                                g_size_cb(g_size_cb_user_data);
                            g_esc_state = ESC_idle;
                            break;
                        }
                        else
                        {
                            if (event_len<MAX_EVENT_BUF-1)
                            {
                                event_buf[event_len++] = ch;
                            }
                        }
                        break;
                }
			}
		}

        if (signals & windowsig)
		{
			struct IntuiMessage *winmsg;
            while (winmsg = (struct IntuiMessage *)GetMsg(g_win->UserPort))
			{
                switch (winmsg->Class)
				{
                    case CLOSEWINDOW:
						running = FALSE;
						break;
                    default:
						break;
				}
                ReplyMsg((struct Message *)winmsg);
			}
		}
	}
    return KEY_CLOSE;
}

void UI_run (void)
{
    BOOL running = TRUE;
    while (running)
    {
        uint16_t key = nextKey();
        if (key == KEY_CLOSE)
            running = FALSE;
        else
            report_key (key);
	}
}

uint16_t UI_EZRequest (char *body, char *gadgets)
{
	ULONG res = rtEZRequest (body, gadgets, NULL, NULL);
	LOG_printf (LOG_DEBUG, "rtEZRequest result: %ld\n", res);
	return res;
}

void UI_setScrollArea (uint16_t row_start, uint16_t row_end)
{
    g_scrollStart = row_start;
    g_scrollEnd   = row_end;
}

void UI_scrollUp (bool fullscreen)
{
    if (!fullscreen)
    {
        //LOG_printf (LOG_DEBUG, "scroll up, g_scrollEnd=%d\n", g_scrollEnd);
        UI_printf ( CSI "%dt", g_scrollEnd);
    }
    UI_printf ( CSI "S");
    if (!fullscreen)
    {
        //UI_printf ( CSI "y");
        UI_printf ( CSI "t");
    }
}

void UI_scrollDown (void)
{
    //LOG_printf (LOG_DEBUG, "scroll down, g_scrollEnd=%d\n", g_scrollEnd);
    //UI_printf ( CSI "%dy", g_scrollStart+1);
    UI_printf ( CSI "%dt", g_scrollEnd);
    UI_printf ( CSI "T");
    //UI_printf ( CSI "y");
    UI_printf ( CSI "t");
}

uint16_t UI_waitkey (void)
{
    return nextKey();
}

void UI_putc(char c)
{
    g_outbuf[g_bpos++] = c;
    if (g_bpos >= BUFSIZE)
        UI_flush();
}

void UI_putstr(char *s)
{
    while (*s)
    {
        g_outbuf[g_bpos++] = *s++;
        if (g_bpos >= BUFSIZE)
            UI_flush();
    }
}

void UI_printf (char* format, ...)
{
    va_list args;
    va_start(args, format);
    UI_vprintf (format, args);
    va_end(args);
}

void UI_vprintf (char* format, va_list args)
{
    static char buf[BUFSIZE];
    vsnprintf (buf, BUFSIZE, format, args);
    UI_putstr(buf);
}

void UI_onSizeChangeCall (UI_size_cb cb, void *user_data)
{
    g_size_cb = cb;
    g_size_cb_user_data = user_data;
}

void UI_moveCursor (int row, int col)
{
    UI_printf (CSI "%d;%d;H", row, col);
}

void UI_eraseToEOL (void)
{
    UI_putstr (CSI "K");
}

void UI_eraseDisplay (void)
{
    UI_moveCursor (1, 1);
    UI_putstr (CSI "J");
}

void UI_bell (void)
{
    UI_putstr ("\007");
}

void UI_setCursorVisible (bool visible)
{
    if (visible)
        UI_putstr ( CSI " p");
    else
        UI_putstr ( CSI "0 p");
}

void UI_setTextStyle (int style)
{
    UI_printf ( CSI "%dm", style);
}

void UI_onKeyCall (UI_key_cb cb, void *user_data)
{
    g_key_cb           = cb;
    g_key_cb_user_data = user_data;
}

#endif

