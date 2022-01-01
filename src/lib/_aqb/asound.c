#include "_aqb.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <devices/audio.h>

#include <graphics/gfxbase.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#define NUM_CHANNELS    4
#define NUM_IO          4

static LONG            g_clock                          = 3579545;    /* Clock constant, 3546895 for PAL */
static struct IOAudio *g_audioIO[NUM_CHANNELS][NUM_IO];
static struct IOAudio *g_cmdIO[NUM_CHANNELS];
static int             g_ion[NUM_CHANNELS];
static struct MsgPort *g_audioMP[NUM_CHANNELS];
static struct MsgPort *g_cmdMP[NUM_CHANNELS];
static ULONG           g_deviceOpen[NUM_CHANNELS];
static WAVE_t         *g_wave[NUM_CHANNELS];
static BOOL            g_ioActive[NUM_CHANNELS][NUM_IO];

static WAVE_t         *g_wave_first = NULL;
static WAVE_t         *g_wave_last  = NULL;
static ULONG           g_zero;

WAVE_t *_wave_alloc (BYTE *data,
                     ULONG oneShotHiSamples, ULONG repeatHiSamples, ULONG samplesPerHiCycle,
                     ULONG samplesPerSec, SHORT ctOctave, FLOAT volume)
{
    WAVE_t *wave = AllocVec(sizeof(*wave), MEMF_CLEAR);
    if (!wave)
    {
        ERROR(AE_AUDIO);
        return NULL;
    }

    wave->prev = g_wave_last;
    if (g_wave_last)
        g_wave_last = g_wave_last->next = wave;
    else
        g_wave_first = g_wave_last = wave;

    wave->data              = data;
    wave->oneShotHiSamples  = oneShotHiSamples;
    wave->repeatHiSamples   = repeatHiSamples;
    wave->samplesPerHiCycle = samplesPerHiCycle;
    wave->samplesPerSec     = samplesPerSec;
    wave->ctOctave          = ctOctave;
    wave->volume            = volume;

    return wave;
}

static BOOL _init_audio_device(int channel)
{
    if (!g_deviceOpen[channel])
        return TRUE;

    if ( (channel < 0) || (channel >= NUM_CHANNELS) )
    {
        DPRINTF ("invalid channel number #%d\n", channel);
        ERROR (AE_AUDIO);
        return FALSE;
    }

    for (int i = 0; i<NUM_IO; i++)
    {
        g_audioIO[channel][i] = (struct IOAudio *) AllocVec ( sizeof (struct IOAudio), MEMF_PUBLIC | MEMF_CLEAR );

        if (!g_audioIO[channel][i])
        {
            DPRINTF ("failed to allocate IOAudio struct\n");
            ERROR (AE_AUDIO);
            return FALSE;
        }
    }
    g_cmdIO[channel] = (struct IOAudio *) AllocVec ( sizeof (struct IOAudio), MEMF_PUBLIC | MEMF_CLEAR );
    if (!g_cmdIO[channel])
    {
        DPRINTF ("failed to allocate IOAudio struct (cmd)\n");
        ERROR (AE_AUDIO);
        return FALSE;
    }

    g_audioMP[channel] = _autil_create_port(0, 0);
    if (!g_audioMP[channel])
    {
        DPRINTF ("failed to create audio message port\n");
        ERROR (AE_AUDIO);
        return FALSE;
    }

    g_cmdMP[channel] = _autil_create_port(0, 0);
    if (!g_cmdMP[channel])
    {
        DPRINTF ("failed to create audio cmd message port\n");
        ERROR (AE_AUDIO);
        return FALSE;
    }

    // create default waveform if there is none

    if (!g_wave[channel])
    {
        BYTE *data = AllocVec(16, MEMF_CHIP | MEMF_CLEAR);
        if (!data)
        {
            ERROR(AE_AUDIO);
            return NULL;
        }

        data[ 0] =    0;
        data[ 1] =   49;
        data[ 2] =   90;
        data[ 3] =  117;
        data[ 4] =  127;
        data[ 5] =  117;
        data[ 6] =   90;
        data[ 7] =   49;
        data[ 8] =    0;
        data[ 9] =  -49;
        data[10] =  -90;
        data[11] = -117;
        data[12] = -127;
        data[13] = -117;
        data[14] =  -90;
        data[15] =  -49;

        WAVE_t *wave = _wave_alloc (data,
                                    /*oneShotHiSamples=*/0, /*repeatHiSamples=*/16, /*samplesPerHiCycle=*/16,
                                    /*samplesPerSec=*/7040, /*ctOctave=*/1, /*volume=*/0x10000);

        g_wave[channel] = wave;
    }

    // allocate channel

    UBYTE whichannel[1] = { 1 << channel };

    g_audioIO[channel][0]->ioa_Request.io_Message.mn_ReplyPort   = g_audioMP[channel];
    g_audioIO[channel][0]->ioa_Request.io_Message.mn_Node.ln_Pri = 0;
    g_audioIO[channel][0]->ioa_Request.io_Command                = ADCMD_ALLOCATE;
    g_audioIO[channel][0]->ioa_Request.io_Flags                  = ADIOF_NOWAIT;
    g_audioIO[channel][0]->ioa_AllocKey                          = 0;
    g_audioIO[channel][0]->ioa_Data                              = whichannel;
    g_audioIO[channel][0]->ioa_Length                            = sizeof(whichannel);

    g_deviceOpen[channel] = OpenDevice ((STRPTR) "audio.device", 0L, (struct IORequest *) g_audioIO[channel][0], 0L);
    if (g_deviceOpen[channel] != 0)
    {
        DPRINTF ("failed to open audio device for channel #%d (g_deviceOpen[channel]=%d)\n", channel, g_deviceOpen[channel]);
        ERROR (AE_AUDIO);
        return FALSE;
    }

    // copy IOs
    for (int i = 1; i<NUM_IO; i++)
        CopyMem (g_audioIO[channel][i-1], g_audioIO[channel][i], sizeof (struct IOAudio));
    CopyMem (g_audioIO[channel][0], g_cmdIO[channel], sizeof (struct IOAudio));

    return TRUE;
}

static void _chan_wait (SHORT channel, SHORT ion)
{
    if ( (channel < 0) || (channel >= NUM_CHANNELS) )
    {
        DPRINTF ("_chan_wait: invalid channel number #%d\n", channel);
        ERROR (AE_AUDIO);
        return ;
    }

    if (!g_ioActive[channel][ion])
        return;

    DPRINTF ("_chan_wait channel=%d...\n", channel);

    WaitPort (g_audioMP[channel]);
    //DPRINTF ("_chan_wait channel=%d... WaitPort() done. GetMsg()...\n", channel);
    struct Message *audioMSG = GetMsg(g_audioMP[channel]);
    //DPRINTF ("_chan_wait channel=%d GetMsg()... done.\n", channel);

    if (!audioMSG)
    {
        DPRINTF ("_chan_wait: failed to get msg, chan #%d\n", channel);
        ERROR (AE_AUDIO);
        return;
    }

    g_ioActive[channel][ion] = FALSE;
}

static void _queueSound (SHORT channel, UWORD period, UWORD samples, UWORD cycles, SHORT vol, BYTE *data)
{
    int ion = g_ion[channel];
    g_ion[channel] = (g_ion[channel]+1) % NUM_IO;

    //DPRINTF ("_queueSound waiting for ion=%d ...\n", ion);

    _chan_wait (channel, ion);

    g_audioIO[channel][ion]->ioa_Request.io_Message.mn_ReplyPort = g_audioMP[channel];
    g_audioIO[channel][ion]->ioa_Request.io_Command              = CMD_WRITE;
    g_audioIO[channel][ion]->ioa_Request.io_Flags                = ADIOF_PERVOL;
    g_audioIO[channel][ion]->ioa_Data                            = (UBYTE *) data;
    g_audioIO[channel][ion]->ioa_Length                          = samples;
    g_audioIO[channel][ion]->ioa_Period                          = period;
    g_audioIO[channel][ion]->ioa_Volume                          = vol;
    g_audioIO[channel][ion]->ioa_Cycles                          = cycles;

    DPRINTF ("_queueSound beginio channel #%d: samples=%d, cycles=%d, period=%d\n", channel, samples, cycles, period);
    //DPRINTF ("_queueSound beginio channel #%d: unit=0x%08lx\n", channel, g_audioIO[channel][ion]->ioa_Request.io_Unit);
    _autil_begin_io ((struct IORequest *) g_audioIO[channel][ion] );
    //SendIO ((struct IORequest *) g_audioIO[channel][ion] );
    g_ioActive[channel][ion] = TRUE;
}


void SOUND (FLOAT freq, FLOAT duration, SHORT vol, SHORT channel)
{
    if (!_init_audio_device(channel))
        return;

    ULONG *dp = (ULONG*) &duration;
    ULONG d = *dp;

    LONG  f                = SPFix(freq);
    ULONG oneShotHiSamples = g_wave[channel]->oneShotHiSamples;
    ULONG repeatHiSamples  = g_wave[channel]->repeatHiSamples;

    //ULONG samples = g_wave[channel]->size;

    //UWORD cycles = SPFix ( SPMul(freq, duration) );                                  // frequency*duration/samcyc
    //UWORD period = SPFix (SPDiv ( SPMul ( SPFlt(samples), freq), SPFlt (g_clock)));  // clock*samcyc/(samples*frequency)

    UWORD period  = f ? SPFix(SPDiv(SPMul(SPFlt(g_wave[channel]->samplesPerHiCycle), freq), SPFlt(g_clock))) : g_clock / g_wave[channel]->samplesPerSec;

    ULONG todoSamples = d != g_zero ? SPFix(SPMul(duration, SPDiv(SPFlt(period), SPFlt(g_clock)))) : oneShotHiSamples+repeatHiSamples;

    DPRINTF("SOUND: period=%d, todoSamples=%d\n", period, todoSamples);

    BYTE *data = g_wave[channel]->data;

    if (todoSamples >= oneShotHiSamples)
    {
        if (oneShotHiSamples)
            _queueSound (channel, period, oneShotHiSamples, /*cycles=*/1, vol, data);

        if (repeatHiSamples)
        {
            data += oneShotHiSamples;
            todoSamples -= oneShotHiSamples;

            UWORD cycles = todoSamples / repeatHiSamples;
            if (cycles<1)
                cycles = 1;

            _queueSound (channel, period, repeatHiSamples, cycles, vol, data);
        }
    }
    else
    {
        _queueSound (channel, period, todoSamples, /*cycles=*/1, vol, data);
    }
}

WAVE_t *WAVE_ (_DARRAY_T *data,
               ULONG oneShotHiSamples, ULONG repeatHiSamples, ULONG samplesPerHiCycle,
               ULONG samplesPerSec, SHORT ctOctave, FLOAT volume)
{
    if (!data || (data->numDims != 1) || (data->elementSize != 1) )
    {
        ERROR(AE_AUDIO);
        return NULL;
    }

    BYTE *d = AllocVec(data->dataSize, MEMF_CHIP | MEMF_CLEAR);
    if (!d)
    {
        ERROR(AE_AUDIO);
        return NULL;
    }

    CopyMem (data->data, d, data->dataSize);

    return _wave_alloc(d, oneShotHiSamples, repeatHiSamples, samplesPerHiCycle, samplesPerSec, ctOctave, volume);
}

void WAVE_FREE (WAVE_t *wave)
{
    if (wave->prev)
        wave->prev->next = wave->next;
    else
        g_wave_first = wave->next;

    if (wave->next)
	{
        wave->next->prev = wave->prev;
	}
    else
	{
        g_wave_last = wave->prev;
	}

	if (wave->data)
	   FreeVec(wave->data);

    FreeVec (wave);
}

void WAVE (SHORT channel, WAVE_t *wave)
{
    if ( !wave || (channel < 0) || (channel >= NUM_CHANNELS) )
    {
        ERROR (AE_AUDIO);
        return ;
    }

    g_wave[channel] = wave;
}

void SOUND_WAIT (SHORT channel)
{
    if (channel<0)
    {
        for (int i = 0; i<NUM_CHANNELS; i++)
        {
            for (int io=0; io<NUM_IO; io++)
                _chan_wait(i, io);
        }
    }
    else
    {
        if ( channel >= NUM_CHANNELS )
        {
            ERROR (AE_AUDIO);
            return ;
        }
        for (int io=0; io<NUM_IO; io++)
            _chan_wait(channel, io);
    }
}

static void _cmd (SHORT channel, UWORD cmd)
{
    if (!_init_audio_device(channel))
        return;

    g_cmdIO[channel]->ioa_Request.io_Message.mn_ReplyPort = g_cmdMP[channel];
    g_cmdIO[channel]->ioa_Request.io_Command              = cmd;
    g_cmdIO[channel]->ioa_Request.io_Flags                = ADIOF_PERVOL;

    g_cmdIO[channel]->ioa_Data                       = NULL;
    g_cmdIO[channel]->ioa_Length                     = 0;
    g_cmdIO[channel]->ioa_Period                     = 200;
    g_cmdIO[channel]->ioa_Volume                     = 1;
    g_cmdIO[channel]->ioa_Cycles                     = 1;

    DPRINTF ("_cmd channel=%d, cmd=%d, unit=0x%08lx...\n", channel, cmd, g_cmdIO[channel]->ioa_Request.io_Unit);

    //_autil_begin_io ((struct IORequest *) g_cmdIO[channel] );
    SendIO ((struct IORequest *) g_cmdIO[channel]);

    WaitPort (g_cmdMP[channel]);
    DPRINTF ("_cmd channel=%d... WaitPort() done. GetMsg()...\n", channel);
    struct Message *cmdMSG = GetMsg(g_cmdMP[channel]);
    DPRINTF ("_cmd channel=%d GetMsg()... done.\n", channel);

    if (!cmdMSG)
    {
        DPRINTF ("_cmd: failed to get msg, chan #%d\n", channel);
        ERROR (AE_AUDIO);
        return;
    }
}

void SOUND_STOP (SHORT channel)
{
    if (channel<0)
    {
        for (int channel = 0; channel<NUM_CHANNELS; channel++)
            _cmd(channel, CMD_STOP);
    }
    else
    {
        if ( channel >= NUM_CHANNELS )
        {
            DPRINTF ("SOUND_STOP: invalid channel number #%d\n", channel);
            ERROR (AE_AUDIO);
            return ;
        }
        _cmd (channel, CMD_STOP);
    }
}

void SOUND_START (SHORT channel)
{
    if (channel<0)
    {
        for (int channel = 0; channel<NUM_CHANNELS; channel++)
            _cmd(channel, CMD_START);
    }
    else
    {
        if ( channel >= NUM_CHANNELS )
        {
            DPRINTF ("SOUND_START: invalid channel number #%d\n", channel);
            ERROR (AE_AUDIO);
            return ;
        }
        _cmd (channel, CMD_START);
    }
}

void _asound_init (void)
{
    for (int channel = 0; channel<NUM_CHANNELS; channel++)
    {
        for (int io=0; io<NUM_IO; io++)
        {
            g_audioIO [channel][io] = NULL;
            g_ioActive[channel][io] = FALSE;
        }
        g_cmdIO[channel]      = NULL;
        g_ion[channel]        = 0;
        g_audioMP[channel]    = NULL;
        g_cmdMP[channel]      = NULL;
        g_deviceOpen[channel] = -1;
        g_wave[channel]       = NULL;
    }

    if (GfxBase->DisplayFlags & PAL)
        g_clock = 3546895;        /* PAL clock */
    else
        g_clock = 3579545;        /* NTSC clock */

    FLOAT z = SPFlt(0);
    ULONG *dp = (ULONG *) &z;
    g_zero = *dp;
}

void _asound_shutdown (void)
{
    for (int i = 0; i<NUM_CHANNELS; i++)
    {
        for (int io=0; io<NUM_IO; io++)
            _chan_wait(i, io);
        if (g_deviceOpen[i] == 0)
            CloseDevice( (struct IORequest *) g_audioIO[i][0] );
        if (g_audioMP[i])
            _autil_delete_port(g_audioMP[i]);
        if (g_cmdMP[i])
            _autil_delete_port(g_cmdMP[i]);
        for (int io=0; io<NUM_IO; io++)
        {
            if (g_audioIO[i][io])
                FreeVec (g_audioIO[i][io]);
        }
        if (g_cmdIO[i])
            FreeVec (g_cmdIO[i]);
    }
    WAVE_t *wave = g_wave_first;
    while (wave)
    {
        WAVE_t *next = wave->next;
        WAVE_FREE(wave);
        wave = next;
    }
}

