#include "link.h"

/*
 * Amiga Hunk file format related definitions
 */


// #define ENABLE_DEBUG

#define HUNK_TYPE_UNIT     0x03E7
#define HUNK_TYPE_NAME     0x03E8
#define HUNK_TYPE_CODE     0x03E9
#define HUNK_TYPE_DATA     0x03EA
#define HUNK_TYPE_BSS      0x03EB
#define HUNK_TYPE_RELOC32  0x03EC
#define HUNK_TYPE_EXT      0x03EF
#define HUNK_TYPE_END      0x03F2
#define HUNK_TYPE_HEADER   0x03F3

#define EXT_TYPE_DEF            1
#define EXT_TYPE_REF32        129

#define MAX_BUF              1024
#define MAX_NUM_HUNKS          16

static uint8_t    g_buf[MAX_BUF];              // scratch buffer
static AS_segment g_hunk_table[MAX_NUM_HUNKS]; // used during hunk object loading
static int        g_hunk_id_cnt;
static AS_segment g_hunk_cur;                  // last code/data/bss hunk read

LI_segmentList LI_SegmentList(void)
{
    LI_segmentList sl = U_poolAlloc (UP_link, sizeof(*sl));

    sl->first = NULL;
    sl->last  = NULL;

    return sl;
}

void LI_segmentListAppend (LI_segmentList sl, AS_segment seg)
{
    LI_segmentListNode node = U_poolAlloc (UP_link, sizeof(*node));

    node->seg  = seg;
    node->next = NULL;

    if (sl->first)
        sl->last = sl->last->next = node;
    else
        sl->first = sl->last = node;
}


static bool fread_u4(FILE *f, uint32_t *u)
{
    if (fread (u, 4, 1, f) != 1)
        return FALSE;

    *u = ENDIAN_SWAP_32 (*u);

    return TRUE;
}

static bool load_hunk_unit(FILE *f)
{
    uint32_t name_len;
    if (!fread_u4 (f, &name_len))
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }
    name_len *=4;
    if (name_len>=MAX_BUF)
    {
        fprintf (stderr, "link: unit name too long.\n");
        return FALSE;
    }

    if (fread (g_buf, name_len, 1, f) != 1)
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }

    g_buf[name_len] = 0;
#ifdef ENABLE_DEBUG
    printf ("link: unit name: %s\n", g_buf);
#endif

    return TRUE;
}

static bool load_hunk_name(FILE *f)
{
    uint32_t name_len;
    if (!fread_u4 (f, &name_len))
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }
    name_len *=4;
    if (name_len>=MAX_BUF)
    {
        fprintf (stderr, "link: hunk name too long.\n");
        return FALSE;
    }

    if (fread (g_buf, name_len, 1, f) != 1)
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }

    g_buf[name_len] = 0;
#ifdef ENABLE_DEBUG
    printf ("link: hunk name: %s\n", g_buf);
#endif

    return TRUE;
}

AS_segment getOrCreateSegment (int id, AS_segKind kind, size_t min_size)
{
    if (id >= MAX_NUM_HUNKS)
    {
        fprintf (stderr, "link: hunk table overflow\n");
        return FALSE;
    }

    AS_segment seg = g_hunk_table[id];

    if (!seg)
    {
        seg = AS_Segment(kind, min_size);
        g_hunk_table[id] = seg;
        return seg;
    }

    if (kind != AS_unknownSeg)
    {
        if (seg->kind == AS_unknownSeg)
            seg->kind = kind;
        if (kind != seg->kind)
            assert(FALSE);
    }

    AS_ensureSegmentSize (seg, min_size);

    return seg;
}

static bool load_hunk_code(FILE *f)
{
    uint32_t code_len;
    if (!fread_u4 (f, &code_len))
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }
    code_len *=4;
#ifdef ENABLE_DEBUG
    printf ("link: code hunk size: %d bytes.\n", code_len);
#endif

    g_hunk_cur = getOrCreateSegment (g_hunk_id_cnt++, AS_codeSeg, code_len);

    if (fread (g_hunk_cur->mem, code_len, 1, f) != 1)
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }
    g_hunk_cur->mem_pos = code_len;

    return TRUE;
}

static bool load_hunk_data(FILE *f)
{
    uint32_t data_len;
    if (!fread_u4 (f, &data_len))
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }
    data_len *=4;
#ifdef ENABLE_DEBUG
    printf ("link: data hunk size: %d bytes.\n", data_len);
#endif

    g_hunk_cur = getOrCreateSegment (g_hunk_id_cnt++, AS_dataSeg, data_len);

    if (fread (g_hunk_cur->mem, data_len, 1, f) != 1)
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }
    g_hunk_cur->mem_pos = data_len;

    return TRUE;
}

static bool load_hunk_bss(FILE *f)
{
    uint32_t bss_len;
    if (!fread_u4 (f, &bss_len))
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }
    bss_len *=4;
#ifdef ENABLE_DEBUG
    printf ("link: bss hunk size: %d bytes.\n", bss_len);
#endif

    g_hunk_cur = getOrCreateSegment (g_hunk_id_cnt++, AS_bssSeg, 0);

    g_hunk_cur->mem_size = bss_len;
    g_hunk_cur->mem_pos  = bss_len;

    return TRUE;
}

static bool load_hunk_reloc32(FILE *f)
{
    uint32_t num_offs, hunk_id;

    while (TRUE)
    {
        if (!fread_u4 (f, &num_offs))
        {
            fprintf (stderr, "link: read error.\n");
            return FALSE;
        }
        if (!num_offs)
            return TRUE; // finished
        if (!fread_u4 (f, &hunk_id))
        {
            fprintf (stderr, "link: read error.\n");
            return FALSE;
        }
#ifdef ENABLE_DEBUG
        printf ("link: reloc32: %d offsets in hunk #%d.\n", num_offs, hunk_id);
#endif

        AS_segment seg = getOrCreateSegment (hunk_id, AS_unknownSeg, /*min_size=*/0);

        for (uint32_t i=0; i<num_offs; i++)
        {
            uint32_t off;
            if (!fread_u4 (f, &off))
            {
                fprintf (stderr, "link: read error.\n");
                return FALSE;
            }
            AS_segmentAddReloc32 (seg, off);
        }
    }

    return TRUE;
}

static bool load_hunk_ext(FILE *f)
{
    if (!g_hunk_cur)
    {
        fprintf (stderr, "link: ext hunk detected when so segment is defined yet.\n");
        return FALSE;
    }

    while (TRUE)
    {
        uint32_t c;
        if (!fread_u4 (f, &c))
        {
            fprintf (stderr, "link: read error.\n");
            return FALSE;
        }

        if (!c)
            return TRUE;

        uint32_t ext_type = c >>24;
        uint32_t name_len = c & 0x00FFFFFF;

        name_len *=4;
        if (name_len>=MAX_BUF)
        {
            fprintf (stderr, "link: hunk name too long.\n");
            return FALSE;
        }

        if (fread (g_buf, name_len, 1, f) != 1)
        {
            fprintf (stderr, "link: read error.\n");
            return FALSE;
        }

        g_buf[name_len] = 0;
#ifdef ENABLE_DEBUG
        printf ("link: hunk_ext: ext_type=%d, name_len=%d, name=%s\n", ext_type, name_len, g_buf);
#endif

        S_symbol sym = S_Symbol ((string) g_buf, /*case_sensitive=*/TRUE);

        switch (ext_type)
        {
            case EXT_TYPE_REF32:
            {
                uint32_t num_refs;
                if (!fread_u4 (f, &num_refs))
                {
                    fprintf (stderr, "link: read error.\n");
                    return FALSE;
                }
                for (uint32_t i=0; i<num_refs; i++)
                {
                    uint32_t offset;
                    if (!fread_u4 (f, &offset))
                    {
                        fprintf (stderr, "link: read error.\n");
                        return FALSE;
                    }

                    AS_segmentAddRef (g_hunk_cur, sym, offset, Temp_w_L);
                }
                break;
            }
            case EXT_TYPE_DEF:
            {
                uint32_t offset;
                if (!fread_u4 (f, &offset))
                {
                    fprintf (stderr, "link: read error.\n");
                    return FALSE;
                }
                AS_segmentAddDef (g_hunk_cur, sym, offset);
                break;
            }
            default:
                fprintf (stderr, "link: FIXME: ext type %d not implemented yet.\n", ext_type);
                assert(FALSE);
        }
    }

    return TRUE;
}

bool LI_segmentListReadObjectFile (LI_segmentList sl, FILE *f)
{
    uint32_t ht;
    if (!fread_u4 (f, &ht))
    {
        fprintf (stderr, "link: read error.\n");
        return FALSE;
    }
#ifdef ENABLE_DEBUG
    printf ("link: hunk type: %08x\n", ht);
#endif

    if (ht != HUNK_TYPE_UNIT)
    {
        fprintf (stderr, "link: This is not an object file, header mismatch: found 0x%08x, expected %08x\n", ht, HUNK_TYPE_UNIT);
    }

    if (!load_hunk_unit(f))
        return FALSE;

    for (int i=0; i<MAX_NUM_HUNKS; i++)
        g_hunk_table[i] = NULL;
    g_hunk_id_cnt = 0;
    g_hunk_cur = NULL;

    while (TRUE)
    {
        if (!fread_u4 (f, &ht))
            break;
#ifdef ENABLE_DEBUG
        printf ("link: hunk type: %08x\n", ht);
#endif

        switch (ht)
        {
            case HUNK_TYPE_UNIT:
                fprintf (stderr, "link: extra unit hunk detected.\n");
                return FALSE;
            case HUNK_TYPE_NAME:
                if (!load_hunk_name(f))
                    return FALSE;
                break;
            case HUNK_TYPE_CODE:
                if (!load_hunk_code(f))
                    return FALSE;
                break;
            case HUNK_TYPE_DATA:
                if (!load_hunk_data(f))
                    return FALSE;
                break;
            case HUNK_TYPE_BSS:
                if (!load_hunk_bss(f))
                    return FALSE;
                break;
            case HUNK_TYPE_RELOC32:
                if (!load_hunk_reloc32(f))
                    return FALSE;
                break;
            case HUNK_TYPE_EXT:
                if (!load_hunk_ext(f))
                    return FALSE;
                break;
            case HUNK_TYPE_END:
                if (!g_hunk_cur)
                {
                    fprintf (stderr, "link: hunk_end detected when no hunk was defined.\n");
                    return FALSE;
                }
                LI_segmentListAppend (sl, g_hunk_cur);
                g_hunk_cur = NULL;
                break;
            default:
                fprintf (stderr, "link: unknown hunk type 0x%08x.\n", ht);
                assert(FALSE);
                return FALSE;
        }
    }
    return TRUE;
}

static bool fwrite_u4(FILE *f, uint32_t u)
{
    u = ENDIAN_SWAP_32 (u);
    if (fwrite (&u, 4, 1, f) != 1)
        return FALSE;
    return TRUE;
}

static bool write_hunk_header (LI_segmentList sl, FILE *f)
{
    if (!fwrite_u4 (f, HUNK_TYPE_HEADER))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }

    if (!fwrite_u4 (f, 0))  // no hunk names
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }

    uint32_t hunkCnt = 0;
    for (LI_segmentListNode n = sl->first; n; n=n->next)
        hunkCnt++;
    if (!fwrite_u4 (f, hunkCnt))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }
    if (!fwrite_u4 (f, 0))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }
    if (!fwrite_u4 (f, hunkCnt-1))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }
    for (LI_segmentListNode n = sl->first; n; n=n->next)
    {
        uint32_t nw = (n->seg->mem_pos + (n->seg->mem_pos % 4)) / 4;
        if (!fwrite_u4 (f, nw))
        {
            fprintf (stderr, "link: write error.\n");
            return FALSE;
        }
    }
    return TRUE;
}

bool write_hunk_code (AS_segment seg, FILE *f)
{
    if (!fwrite_u4 (f, HUNK_TYPE_CODE))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }

    uint32_t  n = (seg->mem_pos + (seg->mem_pos % 4)) / 4;
    printf ("link: writing code section, size= %zd = 0x%08zx bytes (%d = 0x%08x LONGs)\n", seg->mem_pos, seg->mem_pos, n, n);
    if (!fwrite_u4 (f, n))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }
#if 1
    if (fwrite (seg->mem, n*4, 1, f) != 1)
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }
#else
    uint32_t *p = (uint32_t *) seg->mem;
    for (uint32_t i =0; i<n; i++)
    {
        if (!fwrite_u4 (f, *p++))
        {
            fprintf (stderr, "link: write error.\n");
            return FALSE;
        }
    }
#endif
    return TRUE;
}

bool write_hunk_data (AS_segment seg, FILE *f)
{
    if (!fwrite_u4 (f, HUNK_TYPE_DATA))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }

    uint32_t  n = (seg->mem_pos + (seg->mem_pos % 4)) / 4;
    if (!fwrite_u4 (f, n))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }
    uint32_t *p = (uint32_t *) seg->mem;
    for (uint32_t i =0; i<n; i++)
    {
        if (!fwrite_u4 (f, *p++))
        {
            fprintf (stderr, "link: write error.\n");
            return FALSE;
        }
    }
    return TRUE;
}

bool write_hunk_bss (AS_segment seg, FILE *f)
{
    if (!fwrite_u4 (f, HUNK_TYPE_BSS))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }

    uint32_t  n = (seg->mem_pos + (seg->mem_pos % 4)) / 4;
    if (!fwrite_u4 (f, n))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }
    return TRUE;
}

bool write_hunk_end (FILE *f)
{
    if (!fwrite_u4 (f, HUNK_TYPE_END))
    {
        fprintf (stderr, "link: write error.\n");
        return FALSE;
    }
    return TRUE;
}

bool LI_segmentListWriteLoadFile (LI_segmentList sl, FILE *f)
{
    write_hunk_header (sl, f);

    for (LI_segmentListNode n = sl->first; n; n=n->next)
    {
        switch (n->seg->kind)
        {
            case AS_codeSeg:
                if (!write_hunk_code (n->seg, f))
                    return FALSE;
                // FIXME: reloc!
                if (!write_hunk_end (f))
                    return FALSE;
                break;
            case AS_dataSeg:
                if (!write_hunk_data (n->seg, f))
                    return FALSE;
                // FIXME: reloc!
                if (!write_hunk_end (f))
                    return FALSE;
                break;
            case AS_bssSeg:
                if (!write_hunk_bss (n->seg, f))
                    return FALSE;
                // FIXME: reloc!
                if (!write_hunk_end (f))
                    return FALSE;
                break;
            default:
                fprintf (stderr, "***error: unknown segment kind %d !\n", n->seg->kind);
                fflush (f);
                assert(FALSE);
        }
    }
    return FALSE;
}

