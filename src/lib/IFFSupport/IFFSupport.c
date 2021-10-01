
#include "IFFSupport.h"

#include <clib/dos_protos.h>
#include <inline/dos.h>

#define MakeID(a,b,c,d)  ( (LONG)(a)<<24L | (LONG)(b)<<16L | (c)<<8 | (d) )
#define IFF_FORM MakeID('F','O','R','M')
#define IFF_ILBM MakeID('I','L','B','M')
#define IFF_BMHD MakeID('B','M','H','D')
#define IFF_BODY MakeID('B','O','D','Y')

static ULONG _ILBM_SEEK_CHUNK (struct FileHandle *fh, ULONG chunkID)
{
    ULONG h;
    char hdr[5] = {0,0,0,0,0};

    Seek (MKBADDR(fh), 0, OFFSET_BEGINNING);

    LONG l = Read (MKBADDR(fh), &h, 4);
    if ((l != 4) || (h != IFF_FORM))
    {
        ERROR(AE_IFF);
        return 0l;
    }

    _aio_puts (0, (STRPTR)"_ILBM_SEEK_CHUNK: FORM ok."); _aio_putnl(0);

    // ignore FORM length
    l = Read (MKBADDR(fh), &h, 4);
    if (l != 4)
    {
        ERROR(AE_IFF);
        return 0l;
    }

    l = Read (MKBADDR(fh), &h, 4);
    if ((l != 4) || (h != IFF_ILBM))
    {
        ERROR(AE_IFF);
        return 0l;
    }

    while (TRUE)
    {
        ULONG cid;
        l = Read (MKBADDR(fh), &cid, 4);
        if (l != 4)
        {
            break;
        }
        ULONG *p = (ULONG*)hdr;
        *p = cid;
        _aio_puts (0, (STRPTR)"_ILBM_SEEK_CHUNK: chunk id="); _aio_putu4(0, cid); _aio_puts(0, (STRPTR)hdr); _aio_putnl(0);

        l = Read (MKBADDR(fh), &h, 4);
        if (l != 4)
        {
            ERROR(AE_IFF);
            return 0l;
        }
        if (cid == chunkID)
        {
            _aio_puts (0, (STRPTR)"_ILBM_SEEK_CHUNK: *** CHUNK FOUND ***"); _aio_putnl(0);
            _aio_puts (0, (STRPTR)"_ILBM_SEEK_CHUNK: length: "); _aio_putu4(0, h);_aio_putnl(0);
            return h;
        }
        else
        {
            _aio_puts (0, (STRPTR)"_ILBM_SEEK_CHUNK: skipping "); _aio_putu4(0, h);_aio_putnl(0);
            Seek (MKBADDR(fh), h, OFFSET_CURRENT);
        }
    }

    return 0l;
}

void ILBM_LOAD_BMHD (USHORT fno, ILBM_BitMapHeader_t *pBMHD)
{
    struct FileHandle *fh = _aio_getfh(fno);
    if (!fh)
        return;

    DPRINTF ("ILBM_LOAD_BMHD fno=%d\n", fno);

    ULONG len = _ILBM_SEEK_CHUNK (fh, IFF_BMHD);
    if (len != sizeof (ILBM_BitMapHeader_t))
    {
        ERROR(AE_IFF);
        return;
    }

    ULONG l = Read (MKBADDR(fh), pBMHD, len);
    if (l!=len)
        ERROR(AE_IFF);

    DPRINTF ("ILBM_LOAD_BMHD len=%ld\n", len);
    DPRINTF ("ILBM_LOAD_BMHD pBMHD->w=%d, pBMHD->h=%d\n", pBMHD->w, pBMHD->h);

}

void ILBM_LOAD_BODY (USHORT fno, ILBM_BitMapHeader_t *pBMHD, BlitNode blit)
{
    struct FileHandle *fh = _aio_getfh(fno);
    if (!fh)
        return;

    DPRINTF ("ILBM_LOAD_BODY fno=%d\n", fno);

    DPRINTF ("ILBM_LOAD_BODY: pBMHD: %d x %d : %d \n",
             (signed int) pBMHD->w, (signed int) pBMHD->h, (signed int) pBMHD->nPlanes);
    DPRINTF ("ILBM_LOAD_BODY: blit : %d x %d : %d \n",
             (signed int) blit->width, (signed int) blit->height, (signed int) blit->bm.Depth);

    if ((blit->width < pBMHD->w) || (blit->height < pBMHD->h) || (blit->bm.Depth != pBMHD->nPlanes))
    {
        DPRINTF ("ILBM_LOAD_BODY: invalid bit dims %d x %d : %d vs %d x %d : %d\n",
                 (signed int) blit->width, (signed int) blit->height, (signed int) blit->bm.Depth,
                 (signed int) pBMHD->w, (signed int) pBMHD->h, (signed int) pBMHD->nPlanes);
        ERROR(AE_IFF);
        return;
    }

    ULONG len = _ILBM_SEEK_CHUNK (fh, IFF_BODY);
    if (!len)
    {
        ERROR(AE_IFF);
        return;
    }

    DPRINTF ("ILBM_LOAD_BODY ALLOCATING BUFFER, len=%d\n", len);

    APTR buf = ALLOCATE_(len, 0);
    if (!buf)
    {
        ERROR(AE_IFF);
        return;
    }

    //ERROR(AE_IFF);
    return;
}

void _IFFSupport_init(void)
{
}

