
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

void _ilbm_read (struct FileHandle *fh, BITMAP_t **bmRef, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette)
{
    char hdr[5] = {0,0,0,0,0};
    ULONG f = MKBADDR(fh);
    ILBM_META_t myMeta;

    if (!pMeta)
        pMeta = &myMeta;
    pMeta->viewMode = 0;

    ULONG cid;
    ULONG clen;

    LONG l = Read (f, &cid, 4);
    if ((l != 4) || (cid != IFF_FORM))
    {
        ERROR(AE_IFF);
        return;
    }

    DPRINTF ("_ilbm_read: FORM ok.\n");

    // ignore FORM length
    l = Read (f, &clen, 4);
    if (l != 4)
    {
        ERROR(AE_IFF);
        return;
    }

    l = Read (f, &cid, 4);
    if ((l != 4) || (cid != IFF_ILBM))
    {
        ERROR(AE_IFF);
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
            ERROR(AE_IFF);
            return;
        }

        DPRINTF ("_ilbm_read: chunk id=%d (%s) len=%d\n", cid, hdr, clen);

        if ( cid == IFF_BMHD)
        {
            if (!clen || (clen > sizeof (ILBM_META_t)) )
            {
                ERROR(AE_IFF);
                return;
            }

            ULONG l = Read (f, pMeta, clen);
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

                ULONG l = Read (f, &pMeta->viewMode, clen);
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

                    ULONG l = Read (f, buf, clen);
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
                    if ( bmRef && (cid == IFF_BODY) )
                    {
                        if (!clen)
                        {
                            ERROR(AE_IFF);
                            return;
                        }
                        DPRINTF ("ILBM_LOAD BODY len=%ld\n", clen);

                        DPRINTF ("ILBM_LOAD BODY: pMeta: %d x %d : %d compression: %d \n",
                                 (signed int) pMeta->w, (signed int) pMeta->h, (signed int) pMeta->nPlanes, pMeta->compression);

                        BITMAP_t *bm = *bmRef;
                        if (!bm)
                        {
                            DPRINTF ("ILBM_LOAD BODY allocating fresh bitmap bmRef=0x%08lx bm=0x%08lx\n", bmRef, bm);
                            bm = BITMAP_ (pMeta->w, pMeta->h, pMeta->nPlanes);
                            *bmRef = bm;
                        }

                        DPRINTF ("ILBM_LOAD BODY: bm : %d x %d : %d \n",
                                 (signed int) bm->width, (signed int) bm->height, (signed int) bm->bm.Depth);

                        if ((bm->width < pMeta->w) || (bm->height < pMeta->h) || (bm->bm.Depth != pMeta->nPlanes))
                        {
                            DPRINTF ("ILBM_LOAD BODY: invalid bm dims %d x %d : %d vs %d x %d : %d\n",
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

                        ULONG l = Read (f, src, clen);
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

                        DPRINTF ("ILBM_LOAD BODY decoding done\n");

                        DEALLOCATE(src);
                    }
                    else
                    {
                        DPRINTF ("_ILBM_LOAD skipping %d bytes\n", clen);
                        Seek (f, clen, OFFSET_CURRENT);
                    }
                }
            }
        }
    }
}

void ILBM_LOAD_BITMAP (STRPTR path, BITMAP_t **bm, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette)
{
    DPRINTF ("ILBM_LOAD_BITMAP path=%s\n", (char*)path);

    struct FileHandle *fh = BADDR(Open (path, MODE_OLDFILE));
    if (!fh)
    {
        DPRINTF ("ILBM_LOAD_BITMAP open failed.\n");
        ERROR(AE_IFF);
        return;
    }

    _ilbm_read (fh, bm, scid, pMeta, pPalette);

    Close (MKBADDR(fh));
}

#if 0
void ILBM_READ_BITMAP (STRPTR path, BITMAP_t **bm, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette)
    struct FileHandle *fh = _aio_getfh(fno);
    if (!fh)
    {
        ERROR(AE_IFF);
        return;
    }

    Seek (MKBADDR(fh), 0, OFFSET_BEGINNING);
#endif

#if 0
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
#endif

void _IFFSupport_init(void)
{
}

