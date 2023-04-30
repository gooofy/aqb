//#define ENABLE_DPRINTF
#include "IFFSupport.h"

#include <clib/dos_protos.h>
#include <inline/dos.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#define MakeID(a,b,c,d)  ( (LONG)(a)<<24L | (LONG)(b)<<16L | (c)<<8 | (d) )
#define IFF_FORM MakeID('F','O','R','M')
#define IFF_ILBM MakeID('I','L','B','M')
#define IFF_BMHD MakeID('B','M','H','D')
#define IFF_CMAP MakeID('C','M','A','P')
#define IFF_CAMG MakeID('C','A','M','G')
#define IFF_BODY MakeID('B','O','D','Y')
#define IFF_8SVX MakeID('8','S','V','X')
#define IFF_VHDR MakeID('V','H','D','R')

#define RowBytes(w)   (((w) + 15) >> 4 << 1)

static inline int roundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

void _ilbm_read (struct FileHandle *fh, BITMAP_t **bmRef, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette, BOOL cont)
{
    char hdr[5] = {0,0,0,0,0};
    ULONG f = MKBADDR(fh);
    ILBM_META_t myMeta;
    PALETTE_t myPalette;

    DPRINTF ("_ilbm_read: fh=0x%08lx, bmRef=0x%08lx, scid=%d, pMeta=0x%08lx, pPalette=0x%08lx\n",
             fh, bmRef, scid, pMeta, pPalette);
    if (!pMeta)
        pMeta = &myMeta;
    DPRINTF ("_ilbm_read: pMeta before: nPlanes=%d, masking=%d, compression=%d, pad1=%d, transparentColor=%d, pageWidth=0x%04x, pageHeight=0x%04x, viewModes=0x%08lx\n",
             (WORD) pMeta->nPlanes, (WORD) pMeta->masking, (WORD) pMeta->compression, (WORD) pMeta->pad1,
             pMeta->transparentColor, pMeta->pageWidth, pMeta->pageHeight, pMeta->viewModes);
    pMeta->viewModes = 0;

    if (!pPalette && (scid >=0))
        pPalette = &myPalette;

    ULONG cid;
    ULONG clen;

    LONG l = Read (f, &cid, 4);
    if ((l != 4) || (cid != IFF_FORM))
    {
        ERROR(ERR_IFF);
        return;
    }

    DPRINTF ("_ilbm_read: FORM ok.\n");

    // ignore FORM length
    l = Read (f, &clen, 4);
    if (l != 4)
    {
        ERROR(ERR_IFF);
        return;
    }

    l = Read (f, &cid, 4);
    if ((l != 4) || (cid != IFF_ILBM))
    {
        ERROR(ERR_IFF);
        return;
    }

    DPRINTF ("_ilbm_read: format is ILBM. good.\n");

    while (TRUE)
    {
        l = Read (f, &cid, 4);
        if (l != 4)
        {
            break;
        }
        ULONG *p = (ULONG*)hdr;
        *p = cid;


        l = Read (f, &clen, 4);
        if (l != 4)
        {
            ERROR(ERR_IFF);
            return;
        }

        DPRINTF ("_ilbm_read: chunk id=%d (%s) len=%d\n", cid, hdr, clen);

        if ( cid == IFF_BMHD)
        {
            if (!clen || (clen > sizeof (ILBM_META_t)) )
            {
                ERROR(ERR_IFF);
                return;
            }

            ULONG l = Read (f, pMeta, clen);
            if (l!=clen)
                ERROR(ERR_IFF);
            DPRINTF ("_ilbm_read: BMHD: w=%d, h=%d, x=%d, y=%d, nPlanes=%d, masking=%d, compression=%d\n",
                     pMeta->w, pMeta->h, pMeta->x, pMeta->y, pMeta->nPlanes, pMeta->masking, pMeta->compression);
        }
        else
        {
            if ( cid == IFF_CAMG )
            {
                if (clen != 4)
                {
                    ERROR(ERR_IFF);
                    return;
                }

                ULONG l = Read (f, &pMeta->viewModes, clen);
                if (l!=clen)
                    ERROR(ERR_IFF);
                DPRINTF ("_ilbm_read: CAMG: viewModes=0x%08lx\n", pMeta->viewModes);
            }
            else
            {
                if ( pPalette && (cid == IFF_CMAP) )
                {
                    if (!clen)
                    {
                        ERROR(ERR_IFF);
                        return;
                    }
                    DPRINTF ("_ilbm_read CMAP len=%ld\n", clen);

                    UBYTE *buf = ALLOCATE_(clen, 0);
                    if (!buf)
                    {
                        ERROR(ERR_IFF);
                        return;
                    }

                    ULONG l = Read (f, buf, clen);
                    if (l!=clen)
                    {
                        DEALLOCATE(buf);
                        ERROR(ERR_IFF);
                        return;
                    }

                    pPalette->numEntries = l/3;
                    UBYTE *p = buf;
                    for (SHORT i=0; i<pPalette->numEntries; i++)
                    {
                        pPalette->colors[i].r = *p++;
                        pPalette->colors[i].g = *p++;
                        pPalette->colors[i].b = *p++;
                    }
                    DEALLOCATE(buf);

                    if (scid >=0)
                        _palette_load (scid, pPalette);
                }
                else
                {
                    if ( bmRef && (cid == IFF_BODY) )
                    {
                        if (!clen)
                        {
                            ERROR(ERR_IFF);
                            return;
                        }
                        DPRINTF ("_ilbm_read BODY len=%ld\n", clen);

                        UBYTE depth = pMeta->masking == mskHasMask ? pMeta->nPlanes-1 : pMeta->nPlanes;

                        DPRINTF ("_ilbm_read BODY: pMeta: %d x %d : %d compression: %d \n",
                                 (signed int) pMeta->w, (signed int) pMeta->h, depth, pMeta->compression);

                        BITMAP_t *bm = *bmRef;
                        if (!bm)
                        {
                            DPRINTF ("_ilbm_read BODY allocating fresh bitmap bmRef=0x%08lx bm=0x%08lx\n", bmRef, bm);
                            bm = BITMAP_ (pMeta->w, pMeta->h, depth, cont);
                            *bmRef = bm;
                        }

                        DPRINTF ("_ilbm_read BODY: bm : %d x %d : %d \n",
                                 (signed int) bm->width, (signed int) bm->height, (signed int) bm->bm.Depth);

                        if ((bm->width < pMeta->w) || (bm->height < pMeta->h) || (bm->bm.Depth != depth))
                        {
                            DPRINTF ("_ilbm_read BODY: invalid bm dims %d x %d : %d vs %d x %d : %d\n",
                                     (signed int) bm->width, (signed int) bm->height, (signed int) bm->bm.Depth,
                                     (signed int) pMeta->w, (signed int) pMeta->h, depth);
                            ERROR(ERR_IFF);
                            return;
                        }

                        DPRINTF ("_ilbm_read BODY ALLOCATING BUFFER, clen=%d\n", clen);

                        BYTE *src = ALLOCATE_(clen, 0);
                        if (!src)
                        {
                            ERROR(ERR_IFF);
                            return;
                        }

                        ULONG l = Read (f, src, clen);
                        if (l!=clen)
                        {
                            DEALLOCATE(src);
                            ERROR(ERR_IFF);
                            return;
                        }

                        SHORT linelen = RowBytes(pMeta->w);

                        for (SHORT i=0; i<pMeta->h; i++)
                        {
                            //DPRINTF ("_ilbm_read i=%d pMeta->h=%d\n", i, pMeta->h);
                            for (SHORT iPlane=0; iPlane<pMeta->nPlanes; iPlane++)
                            {
                                BOOL maskPlane = (pMeta->masking == mskHasMask) && (iPlane==pMeta->nPlanes-1);
                                //DPRINTF ("_ilbm_read    iPlane=%d pMeta->nPlanes=%d\n", iPlane, pMeta->nPlanes);
                                BYTE *dst = (BYTE *)(bm->bm.Planes[iPlane]) + linelen*i;

                                if (pMeta->compression == 1)	// run length encoding
                                {
                                    SHORT rowbytes = linelen;

                                    while (rowbytes>0)
                                    {
                                        //DPRINTF ("_ilbm_read    rowbytes=%d\n", rowbytes);
                                        BYTE n = *src++;

                                        if (n>=0)
                                        {
                                            if (!maskPlane)
                                            {
                                                CopyMem (src, dst, ++n);
                                                dst      += n;
                                            }
                                            else
                                            {
                                                ++n;
                                            }
                                            rowbytes -= n;
                                            src      += n;
                                        }
                                        else
                                        {
                                            if (n != -128)
                                            {
                                                n         = -n+1;
                                                rowbytes -= n;
                                                if (!maskPlane)
                                                {
                                                    _MEMSET (dst, *src++, n);
                                                    dst += n;
                                                }
                                                else
                                                {
                                                    src++;
                                                }
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    if (!maskPlane)
                                    {
                                        CopyMem (src, dst, linelen);
                                        dst += linelen;
                                    }
                                    src += linelen;
                                }
                            }
                        }

                        DPRINTF ("_ilbm_read: BODY decoding done\n");

                        DEALLOCATE(src);
                    }
                    else
                    {
                        DPRINTF ("_ilbm_read: skipping %d bytes\n", clen);
                        Seek (f, clen, OFFSET_CURRENT);
                    }
                }
            }
        }
    }
}

void ILBM_LOAD_BITMAP (STRPTR path, BITMAP_t **bm, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette, BOOL cont)
{
    DPRINTF ("ILBM_LOAD_BITMAP path=%s\n", (char*)path);

    struct FileHandle *fh = BADDR(Open (path, MODE_OLDFILE));
    if (!fh)
    {
        DPRINTF ("ILBM_LOAD_BITMAP open failed.\n");
        ERROR(ERR_IFF);
        return;
    }

    _ilbm_read (fh, bm, scid, pMeta, pPalette, cont);

    Close (MKBADDR(fh));
}

void ILBM_READ_BITMAP (USHORT fno, BITMAP_t **bm, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette, BOOL cont)
{
    struct FileHandle *fh = _aio_getfh(fno);
    if (!fh)
    {
        ERROR(ERR_IFF);
        return;
    }

    _ilbm_read (fh, bm, scid, pMeta, pPalette, cont);
}

typedef struct
{
    WORD left;
    UWORD right;
} fixed_t;

typedef struct
{
    ULONG   oneShotHiSamples,     /* # samples in the high octave 1-shot part */
            repeatHiSamples,      /* # samples in the high octave repeat part */
            samplesPerHiCycle;    /* # samples/cycle in high octave, else 0 */
    UWORD   samplesPerSec;        /* data sampling rate */
    UBYTE   ctOctave,             /* # of octaves of waveforms */
            sCompression;         /* data compression technique used */
    fixed_t volume;               /* playback nominal volume from 0 to Unity
                                   * (full volume). Map this value into
                                   * the output hardware's dynamic range.  */
} VHDR_t;

void _8svx_read (struct FileHandle *fh, WAVE_t **w)
{
    ULONG f = MKBADDR(fh);

    ULONG cid;
    ULONG clen;

    LONG l = Read (f, &cid, 4);
    if ((l != 4) || (cid != IFF_FORM))
    {
        ERROR(ERR_IFF);
        return;
    }

    DPRINTF ("_8svx_read: FORM ok.\n");

    // FORM length
    l = Read (f, &clen, 4);
    if (l != 4)
    {
        ERROR(ERR_IFF);
        return;
    }

    l = Read (f, &cid, 4);
    if ((l != 4) || (cid != IFF_8SVX))
    {
        ERROR(ERR_IFF);
        return;
    }

    DPRINTF ("_8svx_read: format is 8SVX. good.\n");

    VHDR_t meta;
    BOOL   meta_valid = FALSE;

    while (TRUE)
    {
        l = Read (f, &cid, 4);
        if (l != 4)
        {
            break;
        }

        char hdr[5] = {0,0,0,0,0};
        ULONG *p = (ULONG*)hdr;
        *p = cid;

        l = Read (f, &clen, 4);
        if (l != 4)
        {
            ERROR(ERR_IFF);
            return;
        }

        DPRINTF ("_8svx_read: chunk id=%d (%s) len=%d\n", cid, hdr, clen);

        //Delay (150);

        if ( cid == IFF_VHDR)
        {
            if (!clen || (clen > sizeof (VHDR_t)) )
            {
                ERROR(ERR_IFF);
                return;
            }

            ULONG l = Read (f, &meta, clen);
            if (l!=clen)
                ERROR(ERR_IFF);
            DPRINTF ("_8svx_read: VHDR: oneShotHiSamples=%d, repeatHiSamples=%d\n",
                     meta.oneShotHiSamples, meta.repeatHiSamples);
            DPRINTF ("                : samplesPerHiCycle=%d\n",
                     meta.samplesPerHiCycle);
            DPRINTF ("                : samplesPerSec=%d, ctOctave=%d\n",
                     meta.samplesPerSec, meta.ctOctave);
            DPRINTF ("                : sCompression=%d, volume=0x%08lx\n",
                     meta.sCompression, meta.volume);

            if (meta.sCompression)
            {
                DPRINTF ("_8svx_read: sCompression not supported\n");
                ERROR(ERR_IFF);
                return;
            }

            meta_valid = TRUE;
        }
        else
        {
            if (cid == IFF_BODY)
            {

                if (!meta_valid)
                {
                    DPRINTF ("_8svx_read: BODY encountered, but meta data is not valid\n");
                    ERROR(ERR_IFF);
                    return;
                }

                BYTE *data = AllocVec(clen, MEMF_CHIP | MEMF_CLEAR);
                if (!data)
                {
                    ERROR(ERR_IFF);
                    return;
                }
                ULONG l = Read (f, data, clen);
                if (l!=clen)
                    ERROR(ERR_IFF);

                FLOAT volume = SPDiv (SPFlt (0x10000), SPFlt (volume));

                *w = _wave_alloc (data,
                                  meta.oneShotHiSamples, meta.repeatHiSamples, meta.samplesPerHiCycle,
                                  meta.samplesPerSec,    meta.ctOctave, volume);
            }
            else
            {
				clen = roundUp (clen, 2);
                DPRINTF ("_8svx_read skipping %d bytes\n", clen);
                Seek (f, clen, OFFSET_CURRENT);
            }
        }
    }
}

void IFF8SVX_LOAD_WAVE (STRPTR path, WAVE_t **w)
{
    DPRINTF ("IFF8SVX_LOAD_WAVE path=%s\n", (char*)path);

    struct FileHandle *fh = BADDR(Open (path, MODE_OLDFILE));
    if (!fh)
    {
        DPRINTF ("IFF8SVX_LOAD_WAVE open failed.\n");
        ERROR(ERR_IFF);
        return;
    }

    _8svx_read (fh, w);

    Close (MKBADDR(fh));
}

void IFF8SVX_READ_WAVE (USHORT fno, WAVE_t **w)
{
    struct FileHandle *fh = _aio_getfh(fno);
    if (!fh)
    {
        ERROR(ERR_IFF);
        return;
    }

    _8svx_read (fh, w);
}

void _IFFSupport_init(void)
{
}

