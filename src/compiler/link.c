#include <stdlib.h>
#include <string.h>

#include "link.h"
#include "logger.h"

/*
 * Amiga Hunk file format related definitions
 */


#define HUNK_TYPE_UNIT     0x03E7
#define HUNK_TYPE_NAME     0x03E8
#define HUNK_TYPE_CODE     0x03E9
#define HUNK_TYPE_DATA     0x03EA
#define HUNK_TYPE_BSS      0x03EB
#define HUNK_TYPE_RELOC32  0x03EC
#define HUNK_TYPE_EXT      0x03EF
#define HUNK_TYPE_SYMBOL   0x03F0
#define HUNK_TYPE_END      0x03F2
#define HUNK_TYPE_HEADER   0x03F3

#define EXT_TYPE_DEF            1
#define EXT_TYPE_REF32        129
#define EXT_TYPE_COMMON       130

#define MAX_BUF              1024
#define MAX_NUM_HUNKS          16

#define ENABLE_SYMBOL_HUNK

static uint8_t    g_buf[MAX_BUF];              // scratch buffer
static char       g_name[MAX_BUF];             // current hunk name
static AS_segment g_hunk_table[MAX_NUM_HUNKS]; // used during hunk object loading
static int        g_hunk_id_cnt;
static AS_segment g_hunk_cur;                  // last code/data/bss hunk read
static FILE      *g_fObjFile=NULL;             // object file being written
static FILE      *g_fLoadFile=NULL;            // load file being written

static void link_fail (string msg)
{
    LOG_printf (LOG_ERROR, "*** linker error: %s\n", msg);
    assert(FALSE);

    if (g_fLoadFile)
    {
        fclose (g_fLoadFile);
        g_fLoadFile = NULL;
    }

    if (g_fObjFile)
    {
        fclose (g_fObjFile);
        g_fObjFile = NULL;
    }

    exit(127);
}

#if LOG_LEVEL == LOG_DEBUG
//static void hexdump (uint8_t *mem, uint32_t offset, uint32_t num_bytes)
//{
//    LOG_printf (LOG_DEBUG, "HEX: 0x%08x  ", offset);
//    uint32_t cnt=0;
//    uint32_t num_longs = num_bytes >> 2;
//    while (cnt<num_longs)
//    {
//        uint32_t w = *( (uint32_t *) (mem+offset+cnt*4) );
//        LOG_printf (LOG_DEBUG, " 0x%08x", ENDIAN_SWAP_32(w));
//        cnt++;
//    }
//    LOG_printf (LOG_DEBUG, "\n");
//}
#endif

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
    for (int i=0; i<MAX_NUM_HUNKS; i++)
        g_hunk_table[i] = NULL;
    g_hunk_id_cnt = 0;
    g_hunk_cur = NULL;

    uint32_t name_len;
    if (!fread_u4 (f, &name_len))
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }
    name_len *=4;
    if (name_len>=MAX_BUF)
    {
        LOG_printf (LOG_ERROR, "link: unit name too long.\n");
        return FALSE;
    }

    if (fread (g_buf, name_len, 1, f) != 1)
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }

    g_buf[name_len] = 0;
    LOG_printf (LOG_DEBUG, "link: unit name: %s\n", g_buf);

    return TRUE;
}

static bool load_hunk_name(string sourcefn, FILE *f)
{
    uint32_t name_len;
    if (!fread_u4 (f, &name_len))
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }
    name_len *=4;
    if (name_len>=MAX_BUF)
    {
        LOG_printf (LOG_ERROR, "link: hunk name too long.\n");
        return FALSE;
    }

    if (fread (g_name, name_len, 1, f) != 1)
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }

    g_name[name_len] = 0;
    LOG_printf (LOG_DEBUG, "link: %s: hunk name: %s\n", sourcefn, g_name);

    return TRUE;
}

static AS_segment getOrCreateSegment (string sourcefn, string name, int id, AS_segKind kind, size_t min_size)
{
    if (id >= MAX_NUM_HUNKS)
    {
        LOG_printf (LOG_ERROR, "link: hunk table overflow\n");
        return FALSE;
    }

    AS_segment seg = g_hunk_table[id];

    if (!seg)
    {
        seg = AS_Segment(sourcefn, name, kind, min_size);
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

    if (!seg->name && name)
        seg->name = name;

    AS_ensureSegmentSize (seg, min_size);

    return seg;
}

static bool load_hunk_code(string sourcefn, FILE *f)
{
    uint32_t code_len;
    if (!fread_u4 (f, &code_len))
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }
    code_len *=4;
    LOG_printf (LOG_DEBUG, "link: %s: code hunk size: %d bytes.\n", sourcefn, code_len);

    g_hunk_cur = getOrCreateSegment (sourcefn, String(g_name), g_hunk_id_cnt++, AS_codeSeg, code_len);

    strcpy (g_name, "unnamed");

    if (fread (g_hunk_cur->mem, code_len, 1, f) != 1)
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }
    g_hunk_cur->mem_pos = code_len;

    return TRUE;
}

static bool load_hunk_data(string sourcefn, FILE *f)
{
    uint32_t data_len;
    if (!fread_u4 (f, &data_len))
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }
    data_len *=4;
    LOG_printf (LOG_DEBUG, "link: %s: data hunk size: %d bytes.\n", sourcefn, data_len);

    g_hunk_cur = getOrCreateSegment (sourcefn, String(g_name), g_hunk_id_cnt++, AS_dataSeg, data_len);

    strcpy (g_name, "unnamed");

    if (fread (g_hunk_cur->mem, data_len, 1, f) != 1)
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }
    //hexdump (g_hunk_cur->mem, 0, data_len);
    g_hunk_cur->mem_pos = data_len;

    return TRUE;
}

static bool load_hunk_bss(string sourcefn, FILE *f)
{
    uint32_t bss_len;
    if (!fread_u4 (f, &bss_len))
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }
    bss_len *=4;
    LOG_printf (LOG_DEBUG, "link: %s: bss hunk size: %d bytes.\n", sourcefn, bss_len);

    g_hunk_cur = getOrCreateSegment (sourcefn, String(g_name), g_hunk_id_cnt++, AS_bssSeg, 0);

    strcpy (g_name, "unnamed");

    g_hunk_cur->mem_size = bss_len;
    g_hunk_cur->mem_pos  = bss_len;

    return TRUE;
}

static bool load_hunk_reloc32(string sourcefn, FILE *f)
{
    uint32_t num_offs, hunk_id;

    while (TRUE)
    {
        if (!fread_u4 (f, &num_offs))
        {
            LOG_printf (LOG_ERROR, "link: read error.\n");
            return FALSE;
        }
        if (!num_offs)
            return TRUE; // finished
        if (!fread_u4 (f, &hunk_id))
        {
            LOG_printf (LOG_ERROR, "link: read error.\n");
            return FALSE;
        }
        LOG_printf (LOG_DEBUG, "link: %s: reloc32: %d offsets in hunk #%d.\n", sourcefn, num_offs, hunk_id);

        AS_segment seg = getOrCreateSegment (sourcefn, NULL, hunk_id, AS_unknownSeg, /*min_size=*/0);

        for (uint32_t i=0; i<num_offs; i++)
        {
            uint32_t off;
            if (!fread_u4 (f, &off))
            {
                LOG_printf (LOG_ERROR, "link: read error.\n");
                return FALSE;
            }
            AS_segmentAddReloc32 (g_hunk_cur, seg, off);
        }
    }

    return TRUE;
}

static bool load_hunk_ext(string sourcefn, FILE *f)
{
    if (!g_hunk_cur)
    {
        LOG_printf (LOG_ERROR, "link: ext hunk detected when so segment is defined yet.\n");
        assert(FALSE);
        return FALSE;
    }

    while (TRUE)
    {
        uint32_t c;
        if (!fread_u4 (f, &c))
        {
            LOG_printf (LOG_ERROR, "link: read error.\n");
            return FALSE;
        }

        if (!c)
            return TRUE;

        uint32_t ext_type = c >>24;
        uint32_t name_len = c & 0x00FFFFFF;

        name_len *=4;
        if (name_len>=MAX_BUF)
        {
            LOG_printf (LOG_ERROR, "link: hunk name too long.\n");
            return FALSE;
        }

        if (fread (g_buf, name_len, 1, f) != 1)
        {
            LOG_printf (LOG_ERROR, "link: read error.\n");
            return FALSE;
        }

        g_buf[name_len] = 0;
        LOG_printf (LOG_DEBUG, "link: %s: hunk_ext: ext_type=%d, name_len=%d, name=%s\n", sourcefn, ext_type, name_len, g_buf);

        S_symbol sym = S_Symbol ((string) g_buf, /*case_sensitive=*/FALSE);

        switch (ext_type)
        {
            case EXT_TYPE_REF32:
            {
                uint32_t num_refs;
                if (!fread_u4 (f, &num_refs))
                {
                    LOG_printf (LOG_ERROR, "link: read error.\n");
                    return FALSE;
                }
                for (uint32_t i=0; i<num_refs; i++)
                {
                    uint32_t offset;
                    if (!fread_u4 (f, &offset))
                    {
                        LOG_printf (LOG_ERROR, "link: read error.\n");
                        return FALSE;
                    }

                    AS_segmentAddRef (g_hunk_cur, sym, offset, Temp_w_L, /*common_size=*/0);
                }
                break;
            }
            case EXT_TYPE_COMMON:
            {
                uint32_t common_size;
                if (!fread_u4 (f, &common_size))
                {
                    LOG_printf (LOG_ERROR, "link: read error.\n");
                    return FALSE;
                }
                uint32_t num_refs;
                if (!fread_u4 (f, &num_refs))
                {
                    LOG_printf (LOG_ERROR, "link: read error.\n");
                    return FALSE;
                }
                for (uint32_t i=0; i<num_refs; i++)
                {
                    uint32_t offset;
                    if (!fread_u4 (f, &offset))
                    {
                        LOG_printf (LOG_ERROR, "link: read error.\n");
                        return FALSE;
                    }

                    AS_segmentAddRef (g_hunk_cur, sym, offset, Temp_w_L, /*common_size=*/common_size);
                }
                break;
            }
            case EXT_TYPE_DEF:
            {
                uint32_t offset;
                if (!fread_u4 (f, &offset))
                {
                    LOG_printf (LOG_ERROR, "link: read error.\n");
                    return FALSE;
                }
                AS_segmentAddDef (g_hunk_cur, sym, offset);
                LOG_printf (LOG_DEBUG, "link: %s:  -> ext_def, offset=0x%08x\n", sourcefn, offset);
                break;
            }
            default:
                LOG_printf (LOG_ERROR, "link: FIXME: ext type %d not implemented yet.\n", ext_type);
                assert(FALSE);
        }
    }

    return TRUE;
}

bool LI_segmentListReadObjectFile (LI_segmentList sl, string sourcefn, FILE *f)
{
    uint32_t ht;
    if (!fread_u4 (f, &ht))
    {
        LOG_printf (LOG_ERROR, "link: read error.\n");
        return FALSE;
    }
    LOG_printf (LOG_DEBUG, "link: %s: hunk type: %08x\n", sourcefn, ht);

    if (ht != HUNK_TYPE_UNIT)
    {
        LOG_printf (LOG_ERROR, "link: %s: this is not an object file, header mismatch: found 0x%08x, expected %08x\n", sourcefn, ht, HUNK_TYPE_UNIT);
    }

    if (!load_hunk_unit(f))
        return FALSE;

    strcpy (g_name, "unnamed");

    while (TRUE)
    {
        if (!fread_u4 (f, &ht))
            break;
        LOG_printf (LOG_DEBUG, "link: %s: hunk type: %08x\n", sourcefn, ht);

        switch (ht)
        {
            case HUNK_TYPE_UNIT:
                if (!load_hunk_unit(f))
                    return FALSE;
                break;
            case HUNK_TYPE_NAME:
                if (!load_hunk_name(sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_CODE:
                if (!load_hunk_code(sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_DATA:
                if (!load_hunk_data(sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_BSS:
                if (!load_hunk_bss(sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_RELOC32:
                if (!load_hunk_reloc32(sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_EXT:
                if (!load_hunk_ext(sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_END:
                if (!g_hunk_cur)
                {
                    LOG_printf (LOG_ERROR, "link: hunk_end detected when no hunk was defined.\n");
                    return FALSE;
                }
                LI_segmentListAppend (sl, g_hunk_cur);
                g_hunk_cur = NULL;
                break;
            default:
                LOG_printf (LOG_ERROR, "link: unknown hunk type 0x%08x.\n", ht);
                assert(FALSE);
                return FALSE;
        }
    }
    return TRUE;
}

typedef struct symInfo_ *symInfo;
struct symInfo_
{
    AS_segment  seg;
    uint32_t    offset;
};

bool LI_link (LI_segmentList sl)
{
    TAB_table symTable = TAB_empty(UP_link);   // S_symbol -> symInfo

    // pass 1: collect all symbol definitions from all segments,
    //         assign unique hunk_ids

    uint32_t hunk_id = 0;
    for (LI_segmentListNode node = sl->first; node; node=node->next)
    {
        for (AS_segmentDef def = node->seg->defs; def; def=def->next)
        {
            LOG_printf (LOG_DEBUG, "link: pass1: found definition for symbol %-20s (%p): offset=0x%08x at hunk #%02d (%s)\n", S_name (def->sym), def->sym, def->offset, node->seg->hunk_id, node->seg->sourcefn);

            symInfo si = (symInfo) TAB_look (symTable, def->sym);
            if (si)
            {
                char msg[256];
                snprintf(msg, 256, "symbol %s defined more than once!", S_name(def->sym));
                link_fail (msg);
            }

            si         = U_poolAlloc (UP_link, sizeof(*si));
            si->seg    = node->seg;
            si->offset = def->offset;

            TAB_enter (symTable, def->sym, si);
        }
        node->seg->hunk_id = hunk_id++;
    }

    // pass 2: resolve external references

    AS_segment commonSeg = NULL;

    for (LI_segmentListNode node = sl->first; node; node=node->next)
    {
        if (!node->seg->refs)
            continue;
        TAB_iter i = TAB_Iter (node->seg->refs);
        S_symbol sym;
        AS_segmentRef sr;
        while (TAB_next (i, (void **)&sym, (void**) &sr))
        {
            symInfo si = TAB_look (symTable, sym);
            if (!si)
            {
                if (!sr->common_size)
                {
                    LOG_printf (LOG_ERROR, "link: *** ERROR: unresolved symbol %s (%p)\n\n", S_name(sym), sym);
                    return FALSE;
                }
                if (!commonSeg)
                {
                    commonSeg = AS_Segment ("common", "common", AS_dataSeg, 0);
                    commonSeg->hunk_id = hunk_id++;
                    LI_segmentListAppend (sl, commonSeg);
                }

                si = U_poolAlloc (UP_link, sizeof(*si));
                si->seg    = commonSeg;
                si->offset = commonSeg->mem_pos;
                LOG_printf (LOG_DEBUG, "link: symbol %s allocated in common segment at offset 0x%08x, common_size=%zd\n", S_name(sym), si->offset, sr->common_size);

                TAB_enter (symTable, sym, si);

                AS_assembleDataFill (commonSeg, sr->common_size);
            }

            assert (sr->w == Temp_w_L); // FIXME

            while (sr)
            {
                LOG_printf (LOG_DEBUG, "link: pass2: adding reloc32 for symbol %-20s in hunk #%02d at offset 0x%08x -> hunk #%02d, offset 0x%08x\n",
                            S_name (sym), node->seg->hunk_id, sr->offset, si->seg->hunk_id, si->offset);
                uint32_t *p = (uint32_t *) (node->seg->mem+sr->offset);
                *p = ENDIAN_SWAP_32(si->offset);
                AS_segmentAddReloc32 (node->seg, si->seg, sr->offset);
#if LOG_LEVEL == LOG_DEBUG
                //hexdump (node->seg->mem, sr->offset-4, 16);
#endif
                sr = sr->next;
            }

        }
    }

    return TRUE;
}

static void fwrite_u4(FILE *f, uint32_t u)
{
    u = ENDIAN_SWAP_32 (u);
    if (fwrite (&u, 4, 1, f) != 1)
        link_fail ("write error");
}

static void write_hunk_unit (string name, FILE *f)
{
    fwrite_u4 (f, HUNK_TYPE_UNIT);
    uint32_t l = strlen(name);
    uint32_t n = roundUp(l, 4) / 4;
    LOG_printf (LOG_DEBUG, "link: write hunk unit %s, n=%d\n", name, n);
    fwrite_u4 (f, n);
    if (fwrite (name, n*4, 1, f) != 1)
        link_fail ("write error");
}

static void write_hunk_header (LI_segmentList sl, FILE *f)
{
    fwrite_u4 (f, HUNK_TYPE_HEADER);
    fwrite_u4 (f, 0);  // no hunk names

    uint32_t hunkCnt = 0;
    for (LI_segmentListNode n = sl->first; n; n=n->next)
        hunkCnt++;
    fwrite_u4 (f, hunkCnt);
    fwrite_u4 (f, 0);
    fwrite_u4 (f, hunkCnt-1);
    for (LI_segmentListNode n = sl->first; n; n=n->next)
    {
        uint32_t nw = roundUp(n->seg->mem_pos, 4) / 4;
        fwrite_u4 (f, nw);
    }
}

static void write_hunk_name (AS_segment seg, FILE *f)
{
    fwrite_u4 (f, HUNK_TYPE_NAME);

    uint32_t l = strlen(seg->name);
    uint32_t n = roundUp(l, 4) / 4;
    LOG_printf (LOG_DEBUG, "link: write hunk name %s, n=%d\n", seg->name, n);
    fwrite_u4 (f, n);
    if (fwrite (seg->name, n*4, 1, f) != 1)
        link_fail ("write error");
}

static void write_hunk_code (AS_segment seg, FILE *f)
{
    fwrite_u4 (f, HUNK_TYPE_CODE);

    uint32_t n = roundUp(seg->mem_pos, 4) / 4;
    LOG_printf (LOG_DEBUG, "link: code section, size=%zd bytes\n", seg->mem_pos);
    fwrite_u4 (f, n);
    if (fwrite (seg->mem, n*4, 1, f) != 1)
        link_fail ("write error");
}

static void write_hunk_data (AS_segment seg, FILE *f)
{
    fwrite_u4 (f, HUNK_TYPE_DATA);

    uint32_t n = roundUp(seg->mem_pos, 4) / 4;
    LOG_printf (LOG_DEBUG, "link: data section, size=%zd bytes\n", seg->mem_pos);
    fwrite_u4 (f, n);
    if (n)
    {
        if (fwrite (seg->mem, n*4, 1, f) != 1)
            link_fail ("write error");
    }
}

static void write_hunk_bss (AS_segment seg, FILE *f)
{
    fwrite_u4 (f, HUNK_TYPE_BSS);

    uint32_t n = roundUp(seg->mem_pos, 4) / 4;
    fwrite_u4 (f, n);
}

static void write_hunk_reloc32 (AS_segment seg, FILE *f)
{
    if (!seg->relocs)
        return;

    fwrite_u4 (f, HUNK_TYPE_RELOC32);

    TAB_iter segIter = TAB_Iter (seg->relocs);
    AS_segment seg_to;
    AS_segmentReloc32 relocs;
    while (TAB_next (segIter, (void **)&seg_to, (void **)&relocs))
    {
        uint32_t cnt = 0;
        for (AS_segmentReloc32 r = relocs; r; r=r->next)
            cnt++;
        fwrite_u4 (f, cnt);
        fwrite_u4 (f, seg_to->hunk_id);
        uint32_t i = 0;
        for (AS_segmentReloc32 r = relocs; r; r=r->next)
        {
            fwrite_u4 (f, r->offset);
            i++;
        }
    }
    fwrite_u4 (f, 0);  // end marker
}

#ifdef ENABLE_SYMBOL_HUNK
static void write_hunk_symbol (AS_segment seg, FILE *f)
{
    if (!seg->defs)
        return;
    fwrite_u4 (f, HUNK_TYPE_SYMBOL);

    for (AS_segmentDef def = seg->defs; def; def=def->next)
    {
        string name = S_name (def->sym);
        uint32_t l = strlen(name);
        uint32_t n = roundUp(l,4)/4;
        fwrite_u4 (f, n);
        if (fwrite (name, n*4, 1, f) != 1)
            link_fail ("write error");
        fwrite_u4 (f, def->offset);
    }

    fwrite_u4 (f, 0);
}
#endif

static void write_hunk_ext (AS_segment seg, FILE *f)
{
    if (!seg->defs && !seg->refs)
        return;
    fwrite_u4 (f, HUNK_TYPE_EXT);

    if (seg->refs)
    {
        TAB_iter i = TAB_Iter(seg->refs);
        S_symbol sym;
        AS_segmentRef ref;
        while (TAB_next(i, (void **) &sym, (void **)&ref))
        {
            string name = S_name (sym);
            uint32_t l = strlen(name);
            uint32_t n = roundUp(l,4)/4;
            uint32_t c = (EXT_TYPE_REF32<<24) | n;
            fwrite_u4 (f, c);
            if (fwrite (name, n*4, 1, f) != 1)
                link_fail ("write error");

            uint32_t cnt=0;
            for (AS_segmentRef r=ref; r; r=r->next)
                cnt++;
            fwrite_u4 (f, cnt);
            for (AS_segmentRef r=ref; r; r=r->next)
                fwrite_u4 (f, r->offset);
        }
    }

    for (AS_segmentDef def = seg->defs; def; def=def->next)
    {
        string name = S_name (def->sym);
        uint32_t l = strlen(name);
        uint32_t n = roundUp(l,4)/4;
        uint32_t c = (EXT_TYPE_DEF<<24) | n;
        fwrite_u4 (f, c);
        if (fwrite (name, n*4, 1, f) != 1)
            link_fail ("write error");
        fwrite_u4 (f, def->offset);
    }

    fwrite_u4 (f, 0);
}
static void write_hunk_end (FILE *f)
{
    fwrite_u4 (f, HUNK_TYPE_END);
}

void LI_segmentWriteObjectFile (AS_object obj, string objfn)
{
    g_fObjFile = fopen(objfn, "w");
    if (!g_fObjFile)
    {
        LOG_printf (LOG_ERROR, "*** ERROR: failed to open %s for writing.\n\n", objfn);
        exit(128);
    }
    write_hunk_unit (objfn, g_fObjFile);

    write_hunk_name (obj->codeSeg, g_fObjFile);
    write_hunk_code (obj->codeSeg, g_fObjFile);
    write_hunk_reloc32 (obj->codeSeg, g_fObjFile);
    write_hunk_ext (obj->codeSeg, g_fObjFile);
    write_hunk_end (g_fObjFile);

    write_hunk_name (obj->dataSeg, g_fObjFile);
    write_hunk_data (obj->dataSeg, g_fObjFile);
    write_hunk_reloc32 (obj->dataSeg, g_fObjFile);
    write_hunk_ext (obj->dataSeg, g_fObjFile);
    write_hunk_end (g_fObjFile);

    fclose (g_fObjFile);
    g_fObjFile = NULL;
    LOG_printf (LOG_INFO, "link: created load file: %s\n", objfn);
}

void LI_segmentListWriteLoadFile (LI_segmentList sl, string loadfn)
{
    g_fLoadFile = fopen(loadfn, "w");
    if (!g_fLoadFile)
    {
        LOG_printf (LOG_ERROR, "*** ERROR: failed to open %s for writing.\n\n", loadfn);
        exit(128);
    }
    write_hunk_header (sl, g_fLoadFile);

    for (LI_segmentListNode n = sl->first; n; n=n->next)
    {
        switch (n->seg->kind)
        {
            case AS_codeSeg:
                //write_hunk_name (n->seg, g_fLoadFile);
                write_hunk_code (n->seg, g_fLoadFile);
                write_hunk_reloc32 (n->seg, g_fLoadFile);
#ifdef ENABLE_SYMBOL_HUNK
                write_hunk_symbol (n->seg, g_fLoadFile);
#endif
                write_hunk_end (g_fLoadFile);
                break;
            case AS_dataSeg:
                //write_hunk_name (n->seg, g_fLoadFile);
                write_hunk_data (n->seg, g_fLoadFile);
                write_hunk_reloc32 (n->seg, g_fLoadFile);
#ifdef ENABLE_SYMBOL_HUNK
                write_hunk_symbol (n->seg, g_fLoadFile);
#endif
                write_hunk_end (g_fLoadFile);
                break;
            case AS_bssSeg:
                //write_hunk_name (n->seg, g_fLoadFile);
                write_hunk_bss (n->seg, g_fLoadFile);
                write_hunk_reloc32 (n->seg, g_fLoadFile);
#ifdef ENABLE_SYMBOL_HUNK
                write_hunk_symbol (n->seg, g_fLoadFile);
#endif
                write_hunk_end (g_fLoadFile);
                break;
            default:
                LOG_printf (LOG_ERROR, "***error: unknown segment kind %d !\n", n->seg->kind);
                fflush (g_fLoadFile);
                assert(FALSE);
        }
    }

    fclose (g_fLoadFile);
    g_fLoadFile = NULL;
    LOG_printf (LOG_INFO, "link: created load file: %s\n", loadfn);
}

