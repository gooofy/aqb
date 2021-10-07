
#include "IFFSupport.h"

#include <clib/dos_protos.h>
#include <inline/dos.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#define MakeID(a,b,c,d)  ( (LONG)(a)<<24L | (LONG)(b)<<16L | (c)<<8 | (d) )
#define IFF_FORM MakeID('F','O','R','M')
#define IFF_ILBM MakeID('I','L','B','M')
#define IFF_BMHD MakeID('B','M','H','D')
#define IFF_CMAP MakeID('C','M','A','P')
#define IFF_CAMG MakeID('C','A','M','G')
#define IFF_BODY MakeID('B','O','D','Y')

#define RowBytes(w)   (((w) + 15) >> 4 << 1)

void ILBM_LOAD (USHORT fno, ILBM_META_t *pMeta, PALETTE_t *pPalette, BITMAP_t *bm)
{
    char hdr[5] = {0,0,0,0,0};

    if (!pMeta)
    {
        ERROR(AE_IFF);
        return;
    }
    pMeta->viewMode = 0;

    struct FileHandle *fh = _aio_getfh(fno);
    if (!fh)
    {
        ERROR(AE_IFF);
        return;
    }

    Seek (MKBADDR(fh), 0, OFFSET_BEGINNING);

    ULONG cid;
    ULONG clen;

    LONG l = Read (MKBADDR(fh), &cid, 4);
    if ((l != 4) || (cid != IFF_FORM))
    {
        ERROR(AE_IFF);
        return;
    }

    DPRINTF ("_ILBM_LOAD: FORM ok.\n");

    // ignore FORM length
    l = Read (MKBADDR(fh), &clen, 4);
    if (l != 4)
    {
        ERROR(AE_IFF);
        return;
    }

    l = Read (MKBADDR(fh), &cid, 4);
    if ((l != 4) || (cid != IFF_ILBM))
    {
        ERROR(AE_IFF);
        return;
    }

    DPRINTF ("_ILBM_LOAD: format is ILBM. good.\n");

    while (TRUE)
    {
        l = Read (MKBADDR(fh), &cid, 4);
        if (l != 4)
        {
            break;
        }
        ULONG *p = (ULONG*)hdr;
        *p = cid;


        l = Read (MKBADDR(fh), &clen, 4);
        if (l != 4)
        {
            ERROR(AE_IFF);
            return;
        }

        DPRINTF ("_ILBM_LOAD: chunk id=%d (%s) len=%d\n", cid, hdr, clen);

        if ( cid == IFF_BMHD)
        {
            if (!clen || (clen > sizeof (ILBM_META_t)) )
            {
                ERROR(AE_IFF);
                return;
            }

            ULONG l = Read (MKBADDR(fh), pMeta, clen);
            if (l!=clen)
                ERROR(AE_IFF);
        }
        else
        {
            if ( cid == IFF_CAMG )
            {
                if (clen != 4)
                {
                    ERROR(AE_IFF);
                    return;
                }

                ULONG l = Read (MKBADDR(fh), &pMeta->viewMode, clen);
                if (l!=clen)
                    ERROR(AE_IFF);
            }
            else
            {
                if ( pPalette && (cid == IFF_CMAP) )
                {
                    if (!clen)
                    {
                        ERROR(AE_IFF);
                        return;
                    }
                    DPRINTF ("ILBM_LOAD CMAP len=%ld\n", clen);

                    UBYTE *buf = ALLOCATE_(clen, 0);
                    if (!buf)
                    {
                        ERROR(AE_IFF);
                        return;
                    }

                    ULONG l = Read (MKBADDR(fh), buf, clen);
                    if (l!=clen)
                    {
                        DEALLOCATE(buf);
                        ERROR(AE_IFF);
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
                }
                else
                {
                    if ( bm && (cid == IFF_BODY) )
                    {
                        if (!clen)
                        {
                            ERROR(AE_IFF);
                            return;
                        }
                        DPRINTF ("ILBM_LOAD BODY len=%ld\n", clen);

                        DPRINTF ("ILBM_LOAD BODY: pMeta: %d x %d : %d compression: %d \n",
                                 (signed int) pMeta->w, (signed int) pMeta->h, (signed int) pMeta->nPlanes, pMeta->compression);
                        DPRINTF ("ILBM_LOAD BODY: blt : %d x %d : %d \n",
                                 (signed int) bm->width, (signed int) bm->height, (signed int) bm->bm.Depth);

                        if ((bm->width < pMeta->w) || (bm->height < pMeta->h) || (bm->bm.Depth != pMeta->nPlanes))
                        {
                            DPRINTF ("ILBM_LOAD BODY: invalid bit dims %d x %d : %d vs %d x %d : %d\n",
                                     (signed int) bm->width, (signed int) bm->height, (signed int) bm->bm.Depth,
                                     (signed int) pMeta->w, (signed int) pMeta->h, (signed int) pMeta->nPlanes);
                            ERROR(AE_IFF);
                            return;
                        }

                        DPRINTF ("ILBM_LOAD BODY ALLOCATING BUFFER, clen=%d\n", clen);

                        BYTE *src = ALLOCATE_(clen, 0);
                        if (!src)
                        {
                            ERROR(AE_IFF);
                            return;
                        }

                        ULONG l = Read (MKBADDR(fh), src, clen);
                        if (l!=clen)
                        {
                            DEALLOCATE(src);
                            ERROR(AE_IFF);
                            return;
                        }

                        SHORT linelen = RowBytes(pMeta->w);

                        for (SHORT i=0; i<pMeta->h; i++)
                        {
                            //DPRINTF ("ILBM_LOAD i=%d pMeta->h=%d\n", i, pMeta->h);
                            for (SHORT iPlane=0; iPlane<pMeta->nPlanes; iPlane++)
                            {
                                //DPRINTF ("ILBM_LOAD    iPlane=%d pMeta->nPlanes=%d\n", iPlane, pMeta->nPlanes);
                                BYTE *dst = (BYTE *)(bm->bm.Planes[iPlane]) + linelen*i;

                                if (pMeta->compression == 1)	// run length encoding
                                {
                                    SHORT rowbytes = linelen;

                                    while (rowbytes>0)
                                    {
                                        //DPRINTF ("ILBM_LOAD    rowbytes=%d\n", rowbytes);
                                        BYTE n = *src++;

                                        if (n>=0)
                                        {
                                            CopyMem (src, dst, ++n);
                                            rowbytes -= n;
                                            dst      += n;
                                            src      += n;
                                        }
                                        else
                                        {
                                            if (n != -128)
                                            {
                                            n         = -n+1;
                                            rowbytes -= n;
                                            _MEMSET (dst, *src++, n);
                                            dst += n;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    CopyMem (src, dst, linelen);
                                    src += linelen;
                                    dst += linelen;
                                }
                            }
                        }

                        DEALLOCATE(src);
                    }
                    else
                    {
                        DPRINTF ("_ILBM_LOAD skipping %d bytes\n", clen);
                        Seek (MKBADDR(fh), clen, OFFSET_CURRENT);
                    }
                }
            }
        }
    }
}

BITMAP_t *ILBM_LOAD_BITMAP_ (USHORT fno, PALETTE_t *pPalette, int scid)
{

    BITMAP_t     *bm = NULL;
    PALETTE_t     pal;
    ILBM_META_t   meta;

    if (!pPalette)
        pPalette = &pal;

    ILBM_LOAD (fno, &meta, pPalette, NULL);

    DPRINTF ("ILBM_LOAD_BITMAP_ meta: %d x %d : %d \n",
             (signed int) meta.w, (signed int) meta.h, (signed int) meta.nPlanes);

    bm = BITMAP_ (meta.w, meta.h, meta.nPlanes);
    DPRINTF ("ILBM_LOAD_BITMAP_ bitmap: %d x %d bytes per row: %d \n",
                bm->width, bm->height, bm->bm.BytesPerRow);

    ILBM_LOAD (fno, &meta, NULL, bm);

    if (scid >= 0)
        PALETTE_LOAD (pPalette);

    return bm;
}


void _IFFSupport_init(void)
{
}

