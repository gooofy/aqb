
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

void _ilbm_read (struct FileHandle *fh, BITMAP_t **bmRef, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette, BOOL cont)
{
    char hdr[5] = {0,0,0,0,0};
    ULONG f = MKBADDR(fh);
    ILBM_META_t myMeta;
    PALETTE_t myPalette;

    if (!pMeta)
        pMeta = &myMeta;
    pMeta->viewMode = 0;

    if (!pPalette && (scid >=0))
        pPalette = &myPalette;

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
            DPRINTF ("_ilbm_read: BMHD: w=%d, h=%d, x=%d, y=%d, nPlanes=%d, masking=%d, compression=%d\n",
                     pMeta->w, pMeta->h, pMeta->x, pMeta->y, pMeta->nPlanes, pMeta->masking, pMeta->compression);
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

                    if (scid >=0)
                        _palette_load (scid, pPalette);
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

                        UBYTE depth = pMeta->masking == mskHasMask ? pMeta->nPlanes-1 : pMeta->nPlanes;

                        DPRINTF ("ILBM_LOAD BODY: pMeta: %d x %d : %d compression: %d \n",
                                 (signed int) pMeta->w, (signed int) pMeta->h, depth, pMeta->compression);

                        BITMAP_t *bm = *bmRef;
                        if (!bm)
                        {
                            DPRINTF ("ILBM_LOAD BODY allocating fresh bitmap bmRef=0x%08lx bm=0x%08lx\n", bmRef, bm);
                            bm = BITMAP_ (pMeta->w, pMeta->h, depth, cont);
                            *bmRef = bm;
                        }

                        DPRINTF ("ILBM_LOAD BODY: bm : %d x %d : %d \n",
                                 (signed int) bm->width, (signed int) bm->height, (signed int) bm->bm.Depth);

                        if ((bm->width < pMeta->w) || (bm->height < pMeta->h) || (bm->bm.Depth != depth))
                        {
                            DPRINTF ("ILBM_LOAD BODY: invalid bm dims %d x %d : %d vs %d x %d : %d\n",
                                     (signed int) bm->width, (signed int) bm->height, (signed int) bm->bm.Depth,
                                     (signed int) pMeta->w, (signed int) pMeta->h, depth);
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
                                BOOL maskPlane = (pMeta->masking == mskHasMask) && (iPlane==pMeta->nPlanes-1);
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

void ILBM_LOAD_BITMAP (STRPTR path, BITMAP_t **bm, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette, BOOL cont)
{
    DPRINTF ("ILBM_LOAD_BITMAP path=%s\n", (char*)path);

    struct FileHandle *fh = BADDR(Open (path, MODE_OLDFILE));
    if (!fh)
    {
        DPRINTF ("ILBM_LOAD_BITMAP open failed.\n");
        ERROR(AE_IFF);
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
        ERROR(AE_IFF);
        return;
    }

    _ilbm_read (fh, bm, scid, pMeta, pPalette, cont);
}

void ILBM_LOAD_BOB (STRPTR path, BOB_t **bob, SHORT scid, ILBM_META_t *pMeta, PALETTE_t *pPalette)
{
    DPRINTF ("ILBM_LOAD_BOB path=%s\n", (char*)path);

    struct FileHandle *fh = BADDR(Open (path, MODE_OLDFILE));
    if (!fh)
    {
        DPRINTF ("ILBM_LOAD_BOB open failed.\n");
        ERROR(AE_IFF);
        return;
    }

    BITMAP_t *bm=NULL;

    _ilbm_read (fh, &bm, scid, pMeta, pPalette, /*cont=*/TRUE);

    Close (MKBADDR(fh));

    *bob = BOB_(bm);
}

void _IFFSupport_init(void)
{
}

