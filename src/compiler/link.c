#include <stdlib.h>
#include <string.h>

#include "link.h"
#include "logger.h"
#include "compiler.h"
#include "options.h"

/*
 * Amiga Hunk file format related definitions
 *
 * see http://amiga-dev.wikidot.com/file-format:hunk
 */


#define HUNK_TYPE_UNIT     0x03E7
#define HUNK_TYPE_NAME     0x03E8
#define HUNK_TYPE_CODE     0x03E9
#define HUNK_TYPE_DATA     0x03EA
#define HUNK_TYPE_BSS      0x03EB
#define HUNK_TYPE_RELOC32  0x03EC
#define HUNK_TYPE_EXT      0x03EF
#define HUNK_TYPE_SYMBOL   0x03F0
#define HUNK_TYPE_DEBUG    0x03F1
#define HUNK_TYPE_END      0x03F2
#define HUNK_TYPE_HEADER   0x03F3

#define EXT_TYPE_DEF            1
#define EXT_TYPE_ABS            2
#define EXT_TYPE_REF32        129
#define EXT_TYPE_COMMON       130
#define EXT_TYPE_ABSREF16     138

#define MAX_BUF              1024
#define MAX_NUM_HUNKS          64

#define ENABLE_SYMBOL_HUNK

static uint8_t    g_buf[MAX_BUF];              // scratch buffer
static char       g_name[MAX_BUF];             // current hunk name
static AS_segment g_hunk_table[MAX_NUM_HUNKS]; // used during hunk object loading
static uint32_t   g_hunk_sizes[MAX_NUM_HUNKS]; // used during load file loading
static int        g_hunk_id_cnt;
static AS_segment g_hunk_cur;                  // last code/data/bss hunk read
static FILE      *g_fObjFile=NULL;             // object file being written
static FILE      *g_fLoadFile=NULL;            // load file being written

static void link_fail (string msg)
{
    LOG_printf (LOG_ERROR, "*** linker error: %s\n", msg);
    //assert(FALSE);

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

    CO_exit(EXIT_FAILURE);
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

static bool fread_i4(FILE *f, int32_t *i)
{
    if (fread (i, 4, 1, f) != 1)
        return FALSE;

    *i = ENDIAN_SWAP_32 (*i);

    return TRUE;
}

static bool fread_u2(FILE *f, uint16_t *u)
{
    if (fread (u, 2, 1, f) != 1)
        return FALSE;

    *u = ENDIAN_SWAP_16 (*u);

    return TRUE;
}

static bool fread_u1(FILE *f, uint8_t *u)
{
    if (fread (u, 1, 1, f) != 1)
        return FALSE;

    return TRUE;
}

static bool fread_str(FILE *f, char *buf)
{
    uint16_t l;
    if (!fread_u2 (f, &l))
        return FALSE;
    if (fread (buf, l, 1, f) != 1)
        return FALSE;
    buf[l]=0;
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
        LOG_printf (LOG_ERROR, "link: read error #1.\n");
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
        LOG_printf (LOG_ERROR, "link: read error #2.\n");
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
        LOG_printf (LOG_ERROR, "link: read error #3.\n");
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
        LOG_printf (LOG_ERROR, "link: read error #4.\n");
        return FALSE;
    }

    g_name[name_len] = 0;
    LOG_printf (LOG_DEBUG, "link: %s: hunk name: %s\n", sourcefn, g_name);

    return TRUE;
}

static AS_segment getOrCreateSegment (U_poolId pid, string sourcefn, string name, int id, AS_segKind kind, size_t min_size)
{
    if (id >= MAX_NUM_HUNKS)
    {
        LOG_printf (LOG_ERROR, "link: hunk table overflow\n");
        return FALSE;
    }

    AS_segment seg = g_hunk_table[id];

    if (!seg)
    {
        seg = AS_Segment(pid, sourcefn, name, kind, min_size);
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

    AS_ensureSegmentSize (pid, seg, min_size);

    return seg;
}

static bool load_hunk_code(U_poolId pid, string sourcefn, FILE *f)
{
    uint32_t code_len;
    if (!fread_u4 (f, &code_len))
    {
        LOG_printf (LOG_ERROR, "link: read error #5.\n");
        return FALSE;
    }
    code_len *=4;
    LOG_printf (LOG_DEBUG, "link: %s: code hunk size: %d bytes.\n", sourcefn, code_len);

    g_hunk_cur = getOrCreateSegment (pid, sourcefn, String(UP_link, g_name), g_hunk_id_cnt++, AS_codeSeg, code_len);

    strcpy (g_name, "unnamed");

    if (code_len>0)
    {
        if (fread (g_hunk_cur->mem, code_len, 1, f) != 1)
        {
            LOG_printf (LOG_ERROR, "link: read error #6.\n");
            return FALSE;
        }
    }
    g_hunk_cur->mem_pos = code_len;

    return TRUE;
}

static bool load_hunk_data(U_poolId pid, string sourcefn, FILE *f)
{
    uint32_t data_len;
    if (!fread_u4 (f, &data_len))
    {
        LOG_printf (LOG_ERROR, "link: read error #7.\n");
        return FALSE;
    }
    data_len *=4;
    LOG_printf (LOG_DEBUG, "link: %s: data hunk size: %d bytes.\n", sourcefn, data_len);

    g_hunk_cur = getOrCreateSegment (pid, sourcefn, String(UP_link, g_name), g_hunk_id_cnt++, AS_dataSeg, data_len);

    strcpy (g_name, "unnamed");

    if (fread (g_hunk_cur->mem, data_len, 1, f) != 1)
    {
        LOG_printf (LOG_ERROR, "link: read error #8.\n");
        return FALSE;
    }
    //hexdump (g_hunk_cur->mem, 0, data_len);
    g_hunk_cur->mem_pos = data_len;

    return TRUE;
}

static bool load_hunk_bss(U_poolId pid, string sourcefn, FILE *f)
{
    uint32_t bss_len;
    if (!fread_u4 (f, &bss_len))
    {
        LOG_printf (LOG_ERROR, "link: read error #9.\n");
        return FALSE;
    }
    bss_len *=4;
    LOG_printf (LOG_DEBUG, "link: %s: bss hunk size: %d bytes (hdr: %d bytes).\n", sourcefn, bss_len, g_hunk_sizes[g_hunk_id_cnt]*4);

    g_hunk_cur = getOrCreateSegment (pid, sourcefn, String(UP_link, g_name), g_hunk_id_cnt, AS_bssSeg, g_hunk_sizes[g_hunk_id_cnt]*4);
    g_hunk_id_cnt++;

    strcpy (g_name, "unnamed");

    g_hunk_cur->mem_size = bss_len;
    g_hunk_cur->mem_pos  = bss_len;

    return TRUE;
}

static bool load_hunk_reloc32(U_poolId pid, string sourcefn, FILE *f)
{
    uint32_t num_offs, hunk_id;

    while (TRUE)
    {
        if (!fread_u4 (f, &num_offs))
        {
            LOG_printf (LOG_ERROR, "link: read error #10.\n");
            return FALSE;
        }
        if (!num_offs)
            return TRUE; // finished
        if (!fread_u4 (f, &hunk_id))
        {
            LOG_printf (LOG_ERROR, "link: read error #11.\n");
            return FALSE;
        }
        LOG_printf (LOG_DEBUG, "link: %s: reloc32: %d offsets in hunk #%d.\n", sourcefn, num_offs, hunk_id);

        AS_segment seg = getOrCreateSegment (pid, sourcefn, NULL, hunk_id, AS_unknownSeg, /*min_size=*/0);

        for (uint32_t i=0; i<num_offs; i++)
        {
            uint32_t off;
            if (!fread_u4 (f, &off))
            {
                LOG_printf (LOG_ERROR, "link: read error #12.\n");
                return FALSE;
            }
            AS_segmentAddReloc32 (pid, g_hunk_cur, seg, off);
        }
    }

    return TRUE;
}

static bool load_hunk_ext(U_poolId pid, string sourcefn, FILE *f)
{
    if (!g_hunk_cur)
    {
        LOG_printf (LOG_ERROR, "link: ext hunk detected when no segment is defined yet.\n");
        assert(FALSE);
        return FALSE;
    }

    while (TRUE)
    {
        uint32_t c;
        if (!fread_u4 (f, &c))
        {
            LOG_printf (LOG_ERROR, "link: read error #13.\n");
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
            LOG_printf (LOG_ERROR, "link: read error #14.\n");
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
                    LOG_printf (LOG_ERROR, "link: read error #15.\n");
                    return FALSE;
                }
                for (uint32_t i=0; i<num_refs; i++)
                {
                    uint32_t offset;
                    if (!fread_u4 (f, &offset))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #16.\n");
                        return FALSE;
                    }

                    AS_segmentAddRef (pid, g_hunk_cur, sym, offset, Temp_w_L, /*common_size=*/0);
                }
                break;
            }
            case EXT_TYPE_COMMON:
            {
                uint32_t common_size;
                if (!fread_u4 (f, &common_size))
                {
                    LOG_printf (LOG_ERROR, "link: read error #17.\n");
                    return FALSE;
                }
                uint32_t num_refs;
                if (!fread_u4 (f, &num_refs))
                {
                    LOG_printf (LOG_ERROR, "link: read error #18.\n");
                    return FALSE;
                }
                for (uint32_t i=0; i<num_refs; i++)
                {
                    uint32_t offset;
                    if (!fread_u4 (f, &offset))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #19.\n");
                        return FALSE;
                    }

                    AS_segmentAddRef (pid, g_hunk_cur, sym, offset, Temp_w_L, /*common_size=*/common_size);
                }
                break;
            }
            case EXT_TYPE_DEF:
            {
                uint32_t offset;
                if (!fread_u4 (f, &offset))
                {
                    LOG_printf (LOG_ERROR, "link: read error #20.\n");
                    return FALSE;
                }
                AS_segmentAddDef (pid, g_hunk_cur, sym, offset);
                LOG_printf (LOG_DEBUG, "link: %s:  -> ext_def, offset=0x%08x\n", sourcefn, offset);
                break;
            }
            case EXT_TYPE_ABS:
            {
                uint32_t v;
                if (!fread_u4 (f, &v))
                {
                    LOG_printf (LOG_ERROR, "link: read error #21.\n");
                    return FALSE;
                }
                // FIXME AS_segmentAddDef (g_hunk_cur, sym, v);
                LOG_printf (LOG_DEBUG, "link: %s:  -> ext_abs, v=0x%08x\n", sourcefn, v);
                break;
            }
            case EXT_TYPE_ABSREF16:
            {
                uint32_t num_refs;
                if (!fread_u4 (f, &num_refs))
                {
                    LOG_printf (LOG_ERROR, "link: read error #22.\n");
                    return FALSE;
                }
                for (uint32_t i=0; i<num_refs; i++)
                {
                    uint32_t v;
                    if (!fread_u4 (f, &v))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #23.\n");
                        return FALSE;
                    }

                    // FIXME AS_segmentAddRef (g_hunk_cur, sym, v, Temp_w_L, /*common_size=*/0);
                    LOG_printf (LOG_DEBUG, "link: %s:  -> ext_absref16, v=0x%08x\n", sourcefn, v);
                }
                break;
            }
            default:
                LOG_printf (LOG_ERROR, "link: FIXME: ext type %d not implemented yet.\n", ext_type);
                assert(FALSE);
        }
    }

    return TRUE;
}

static bool load_hunk_symbol(U_poolId pid, string sourcefn, FILE *f)
{
    if (!g_hunk_cur)
    {
        LOG_printf (LOG_ERROR, "link: symbol hunk detected when no segment is defined yet.\n");
        assert(FALSE);
        return FALSE;
    }

    while (TRUE)
    {
        uint32_t num_longs;
        if (!fread_u4 (f, &num_longs))
        {
            LOG_printf (LOG_ERROR, "link: read error #32.\n");
            return FALSE;
        }

        if (!num_longs)
            return TRUE;

        uint32_t name_len = num_longs * 4;
        if (name_len>=MAX_BUF)
        {
            LOG_printf (LOG_ERROR, "link: symbol name too long.\n");
            return FALSE;
        }

        if (fread (g_buf, name_len, 1, f) != 1)
        {
            LOG_printf (LOG_ERROR, "link: read error #33.\n");
            return FALSE;
        }

        g_buf[name_len] = 0;

        uint32_t offset;
        if (!fread_u4 (f, &offset))
        {
            LOG_printf (LOG_ERROR, "link: read error #34.\n");
            return FALSE;
        }
        //LOG_printf (LOG_DEBUG, "link: hunk_symbol: name=%s(len=%d) offset=0x%08lx\n", g_buf, name_len, offset);
        S_symbol sym = S_Symbol ((char *)g_buf, FALSE);
        AS_segmentAddDef (pid, g_hunk_cur, sym, offset);
    }

    return TRUE;
}

static bool fread_ty (FILE *f, Ty_ty *ty)
{
    *ty = NULL;
    uint8_t ty_kind;
    if (!fread_u1 (f, &ty_kind))
    {
        LOG_printf (LOG_ERROR, "link: read error #45.\n");
        return FALSE;
    }
    switch (ty_kind)
    {
        case Ty_bool:     *ty = Ty_Bool()    ; break;
        case Ty_byte:     *ty = Ty_Byte()    ; break;
        case Ty_ubyte:    *ty = Ty_UByte()   ; break;
        case Ty_integer:  *ty = Ty_Integer() ; break;
        case Ty_uinteger: *ty = Ty_UInteger(); break;
        case Ty_long:     *ty = Ty_Long()    ; break;
        case Ty_ulong:    *ty = Ty_ULong()   ; break;
        case Ty_single:   *ty = Ty_Single()  ; break;
        case Ty_double:   *ty = Ty_Double()  ; break;

        // FIXME:
        case Ty_sarray:
        case Ty_darray:
        case Ty_record:
        case Ty_pointer:
        case Ty_string:
        case Ty_void:
        case Ty_forwardPtr:
        case Ty_procPtr:
        case Ty_toLoad:
        case Ty_prc:
            LOG_printf (LOG_DEBUG, "link: load_hunk_debug: unknown fvi type kind %d\n", ty_kind);
            break;

     }
    return TRUE;
}

static bool load_hunk_debug(U_poolId pid, string sourcefn, FILE *f)
{
    uint32_t num_longs;
    if (!fread_u4 (f, &num_longs))
    {
        LOG_printf (LOG_ERROR, "link: read error #35.\n");
        return FALSE;
    }

    LOG_printf (LOG_DEBUG, "link: load_hunk_debug num_longs=%d\n", num_longs);

    if (!num_longs)
        return TRUE;

    if (num_longs < 3)
    {
        LOG_printf (LOG_ERROR, "link: debug hunk too short.\n");
        return FALSE;
    }

    int32_t pos_start = ftell(f);

    LOG_printf (LOG_DEBUG, "link: load_hunk_debug: num_longs=%ld, pos_start=%ld\n", num_longs, pos_start);

    uint32_t magic, version;
    if (!fread_u4 (f, &magic))
    {
        LOG_printf (LOG_ERROR, "link: read error #37.\n");
        return FALSE;
    }
    if (!fread_u4 (f, &version))
    {
        LOG_printf (LOG_ERROR, "link: read error #38.\n");
        return FALSE;
    }

    if ((magic != DEBUG_MAGIC) || (version != DEBUG_VERSION))
    {
        LOG_printf (LOG_ERROR, "link: debug hunk format not recognized.\n");
        return FALSE;
    }

    bool finished = FALSE;
    while (!finished)
    {
        uint16_t cmd;
        if (!fread_u2 (f, &cmd))
        {
            LOG_printf (LOG_ERROR, "link: read error #36.\n");
            return FALSE;
        }

        LOG_printf (LOG_DEBUG, "link: load_hunk_debug: cmd=%d\n", cmd);
        switch (cmd)
        {
            case DEBUG_INFO_LINE:
            {
                uint16_t line;
                uint32_t offset;
                if (!fread_u2 (f, &line))
                {
                    LOG_printf (LOG_ERROR, "link: read error #37.\n");
                    return FALSE;
                }
                if (!fread_u4 (f, &offset))
                {
                    LOG_printf (LOG_ERROR, "link: read error #38.\n");
                    return FALSE;
                }
                LOG_printf (LOG_DEBUG, "link: hunk_debug: INFO_LINE line=%4d offset=0x%08lx\n", line, offset);
                AS_segmentAddSrcMap (pid, g_hunk_cur, line, offset);
                break;
            }
            case DEBUG_INFO_FRAME:
            {
                uint32_t code_start, code_end;
                if (!fread_str (f, (char *) g_buf))
                {
                    LOG_printf (LOG_ERROR, "link: read error #39.\n");
                    return FALSE;
                }
                if (!fread_u4 (f, &code_start))
                {
                    LOG_printf (LOG_ERROR, "link: read error #41.\n");
                    return FALSE;
                }
                if (!fread_u4 (f, &code_end))
                {
                    LOG_printf (LOG_ERROR, "link: read error #42.\n");
                    return FALSE;
                }
                LOG_printf (LOG_DEBUG, "link: hunk_debug: INFO_FRAME label=%s, code_start=0x%08lx, code_end=0x%08lx\n", g_buf, code_start, code_end);
                AS_frameMapNode fmn = AS_segmentAddFrameMap (pid, g_hunk_cur, Temp_namedlabel((char *) g_buf), code_start, code_end);

                uint16_t cnt;
                if (!fread_u2 (f, &cnt))
                {
                    LOG_printf (LOG_ERROR, "link: read error #43.\n");
                    return FALSE;
                }
                for (uint16_t i=0; i<cnt; i++)
                {
                    int32_t offset;
                    Ty_ty ty = NULL;
                    if (!fread_str (f, (char *) g_buf))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #44.\n");
                        return FALSE;
                    }
                    if (!fread_ty (f, &ty))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #45.\n");
                        return FALSE;
                    }
                    if (!fread_i4 (f, &offset))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #46.\n");
                        return FALSE;
                    }
                    LOG_printf (LOG_DEBUG, "link: load_hunk_debug: frame var info name=%s, offset=%d\n", g_buf, offset);
                     if (ty)
                         AS_frameMapAddFVI (pid, fmn, S_Symbol((char *)g_buf, FALSE), ty, offset);
                }
                break;
            }
            case DEBUG_INFO_GLOBAL_VARS:
            {
                uint16_t cnt;
                if (!fread_u2 (f, &cnt))
                {
                    LOG_printf (LOG_ERROR, "link: read error #47.\n");
                    return FALSE;
                }
                for (uint16_t i=0; i<cnt; i++)
                {
                    Temp_label label;
                    Ty_ty ty = NULL;
                    uint32_t offset;
                    if (!fread_str (f, (char *) g_buf))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #46.\n");
                        return FALSE;
                    }
                    label = S_Symbol ((char *)g_buf, FALSE);
                    if (!fread_ty (f, &ty))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #45.\n");
                        return FALSE;
                    }
                    if (!fread_u4 (f, &offset))
                    {
                        LOG_printf (LOG_ERROR, "link: read error #48.\n");
                        return FALSE;
                    }
                    if (ty)
                        AS_segmentAddGVI (pid, g_hunk_cur, label, ty, offset);
                }
                break;
            }
            case DEBUG_INFO_END:
                finished = TRUE;
                break;
            default:
                LOG_printf (LOG_ERROR, "link: debug hunk: unknown entry 0x%04x\n", cmd);
                return FALSE;
        }
    }

    // read filler bytes, if any
    int32_t pos_end = ftell(f);
    int32_t size = pos_end - pos_start;
    int32_t size4 = num_longs*4;
    LOG_printf (LOG_DEBUG, "link: load_hunk_debug: size=%ld, size4=%d\n", size, size4);
    while (size<size4)
    {
        uint8_t u;
        if (!fread_u1 (f, &u))
        {
            LOG_printf (LOG_ERROR, "link: read error #39.\n");
            return FALSE;
        }
        LOG_printf (LOG_DEBUG, "link: load_hunk_debug: filler read: %d\n", u);
        size++;
    }

    return TRUE;
}

bool LI_segmentListReadObjectFile (U_poolId pid, LI_segmentList sl, string sourcefn, FILE *f)
{
    uint32_t ht;
    if (!fread_u4 (f, &ht))
    {
        LOG_printf (LOG_ERROR, "link: read error #24.\n");
        return FALSE;
    }
    LOG_printf (LOG_DEBUG, "link: %s: hunk type: %08x\n", sourcefn, ht);

    if (ht != HUNK_TYPE_UNIT)
    {
        LOG_printf (LOG_ERROR, "link: %s: this is not an object file, header mismatch: found 0x%08x, expected %08x\n", sourcefn, ht, HUNK_TYPE_UNIT);
        return FALSE;
    }

    for (uint32_t i = 0; i<MAX_NUM_HUNKS; i++)
        g_hunk_sizes[i] = 0;

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
                if (!load_hunk_code(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_DATA:
                if (!load_hunk_data(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_BSS:
                if (!load_hunk_bss(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_RELOC32:
                if (!load_hunk_reloc32(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_EXT:
                if (!load_hunk_ext(pid, sourcefn, f))
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

bool LI_link (U_poolId pid, LI_segmentList sl)
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
                    commonSeg = AS_Segment (pid, "common", "common", AS_dataSeg, 0);
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
                AS_segmentAddReloc32 (pid, node->seg, si->seg, sr->offset);
#if LOG_LEVEL == LOG_DEBUG
                //hexdump (node->seg->mem, sr->offset-4, 16);
#endif
                sr = sr->next;
            }

        }
    }

    return TRUE;
}

static void fwrite_u1(FILE *f, uint8_t u)
{
    if (fwrite (&u, 1, 1, f) != 1)
        link_fail ("write error");
}

static void fwrite_u2(FILE *f, uint16_t u)
{
    u = ENDIAN_SWAP_16 (u);
    if (fwrite (&u, 2, 1, f) != 1)
        link_fail ("write error");
}

static void fwrite_u4(FILE *f, uint32_t u)
{
    u = ENDIAN_SWAP_32 (u);
    if (fwrite (&u, 4, 1, f) != 1)
        link_fail ("write error");
}

static void fwrite_i4(FILE *f, int32_t i)
{
    i = ENDIAN_SWAP_32 (i);
    if (fwrite (&i, 4, 1, f) != 1)
        link_fail ("write error");
}

static void fwrite_str(FILE *f, char *str)
{
    uint16_t l = strlen(str);
    fwrite_u2 (f, l);
    if (fwrite (str, l, 1, f) != 1)
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
    if (n>0)
    {
        if (fwrite (seg->mem, n*4, 1, f) != 1)
            link_fail ("write error");
    }
}

static void write_hunk_data (AS_segment seg, FILE *f)
{
    fwrite_u4 (f, HUNK_TYPE_DATA);

    uint32_t n = roundUp(seg->mem_pos, 4) / 4;
    LOG_printf (LOG_DEBUG, "link: data section, size=%zd bytes\n", seg->mem_pos);
    fwrite_u4 (f, n);
    if (n>0)
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
        if (n)
        {
            uint32_t todo=n*4;
            if (fwrite (name, l, 1, f) != 1)
                link_fail ("write error");
            todo -= l;
            while (todo--)
                fwrite_u1 (f, 0);
        }
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
            if (n>0)
            {
                uint32_t todo=n*4;
                if (fwrite (name, l, 1, f) != 1)
                    link_fail ("write error");
                todo -= l;
                while (todo--)
                    fwrite_u1 (f, 0);
            }

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
        if (n>0)
        {
            uint32_t todo=n*4;
            if (fwrite (name, l, 1, f) != 1)
                link_fail ("write error");
            todo -= l;
            while (todo--)
                fwrite_u1 (f, 0);
        }
        fwrite_u4 (f, def->offset);
    }

    fwrite_u4 (f, 0);
}

static void write_hunk_debug (AS_segment seg, FILE *f)
{
    LOG_printf (LOG_DEBUG, "link: write_hunk_debug starts\n");

    if (!seg->srcMap && !seg->frameMap && !seg->globals)
    {
        LOG_printf (LOG_DEBUG, "link: write_hunk_debug no debug info on this segment -> returning\n");
        return;
    }

    fwrite_u4 (f, HUNK_TYPE_DEBUG);

    LOG_printf (LOG_DEBUG, "link: write_hunk_debug: debug section starts\n");
    fwrite_u4 (f, 0);   // we will fit that in once we're done writing
    int32_t start_pos = ftell(f);

    fwrite_u4 (f, DEBUG_MAGIC);
    fwrite_u4 (f, DEBUG_VERSION);

    for (AS_srcMapNode n = seg->srcMap; n; n=n->next)
    {
        fwrite_u2 (f, DEBUG_INFO_LINE);
        fwrite_u2 (f, n->line);
        fwrite_u4 (f, n->offset);
    }

    for (AS_frameMapNode n = seg->frameMap; n; n=n->next)
    {
        char *name = S_name (n->label);
        LOG_printf (LOG_DEBUG, "link: write_hunk_debug: frame map entry name=%s, code_start=%d, code_end=%d\n", name, n->code_start, n->code_end);

        fwrite_u2 (f, DEBUG_INFO_FRAME);
        fwrite_str (f, name);
        fwrite_u4 (f, n->code_start);
        fwrite_u4 (f, n->code_end);

        uint16_t cnt = 0;
        for (AS_frameVarNode m = n->vars; m; m=m->next)
            cnt++;
        fwrite_u2 (f, cnt);
        for (AS_frameVarNode m = n->vars; m; m=m->next)
        {
            char *vname = S_name (m->sym);
            LOG_printf (LOG_DEBUG, "link: write_hunk_debug: frame var info name=%s, type=%d, offset=%d\n", vname, m->ty->kind, m->offset);
            fwrite_str (f, vname);
            fwrite_u1  (f, m->ty->kind);
            fwrite_i4  (f, m->offset);
        }
    }

    fwrite_u2 (f, DEBUG_INFO_GLOBAL_VARS);
    uint16_t cnt = 0;
    for (AS_globalVarNode m = seg->globals; m; m=m->next)
        cnt++;
    fwrite_u2 (f, cnt);
    for (AS_globalVarNode n = seg->globals; n; n=n->next)
    {
        char *label = S_name (n->label);
        LOG_printf (LOG_DEBUG, "link: write_hunk_debug: global var info type=%d, label=%s, offset=%d\n", n->ty->kind, label, n->offset);
        fwrite_str (f, label);
        fwrite_u1  (f, n->ty->kind);
        fwrite_u4  (f, n->offset);
    }

    fwrite_u2 (f, DEBUG_INFO_END);

    int32_t end_pos = ftell(f);

    int32_t size = end_pos-start_pos;
    int32_t size4 = roundUp(size, 4);

    LOG_printf (LOG_DEBUG, "link: write_hunk_debug: debug section: finishing, size=%ld, size4=%ld\n", size, size4);

    while (size<size4)
    {
        fwrite_u1 (f, 0);
        size++;
    }

    fseek (f, -size4-4, SEEK_CUR);
    fwrite_u4 (f, size4/4);
    fseek (f, size4, SEEK_CUR);
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
        CO_exit(EXIT_FAILURE);
    }
    write_hunk_unit (objfn, g_fObjFile);

    write_hunk_name    (obj->codeSeg, g_fObjFile);
    write_hunk_code    (obj->codeSeg, g_fObjFile);
    write_hunk_reloc32 (obj->codeSeg, g_fObjFile);
    write_hunk_ext     (obj->codeSeg, g_fObjFile);
    if (OPT_get (OPTION_DEBUG))
        write_hunk_debug   (obj->codeSeg, g_fObjFile);
    write_hunk_end     (g_fObjFile);

    write_hunk_name    (obj->dataSeg, g_fObjFile);
    write_hunk_data    (obj->dataSeg, g_fObjFile);
    write_hunk_reloc32 (obj->dataSeg, g_fObjFile);
    write_hunk_ext     (obj->dataSeg, g_fObjFile);
    if (OPT_get (OPTION_DEBUG))
        write_hunk_debug   (obj->dataSeg, g_fObjFile);
    write_hunk_end     (g_fObjFile);

    fclose (g_fObjFile);
    g_fObjFile = NULL;
    LOG_printf (LOG_INFO, "link: created object file: %s\n", objfn);
}

void LI_segmentListWriteLoadFile (LI_segmentList sl, string loadfn)
{
    LOG_printf (LOG_INFO, "link: creating load file: %s\n", loadfn);
    //U_delay(1000);

    g_fLoadFile = fopen(loadfn, "w");
    if (!g_fLoadFile)
    {
        LOG_printf (LOG_ERROR, "*** ERROR: failed to open %s for writing.\n\n", loadfn);
        CO_exit(EXIT_FAILURE);
    }
    write_hunk_header (sl, g_fLoadFile);

    uint32_t hunk_id = 0;
    for (LI_segmentListNode n = sl->first; n; n=n->next)
    {
        assert (n->seg->hunk_id == hunk_id++);
        switch (n->seg->kind)
        {
            case AS_codeSeg:
                //write_hunk_name (n->seg, g_fLoadFile);
                write_hunk_code (n->seg, g_fLoadFile);
                write_hunk_reloc32 (n->seg, g_fLoadFile);
#ifdef ENABLE_SYMBOL_HUNK
                write_hunk_symbol (n->seg, g_fLoadFile);
#endif
                if (OPT_get (OPTION_DEBUG))
                    write_hunk_debug (n->seg, g_fLoadFile);
                write_hunk_end (g_fLoadFile);
                break;
            case AS_dataSeg:
                //write_hunk_name (n->seg, g_fLoadFile);
                write_hunk_data (n->seg, g_fLoadFile);
                write_hunk_reloc32 (n->seg, g_fLoadFile);
#ifdef ENABLE_SYMBOL_HUNK
                write_hunk_symbol (n->seg, g_fLoadFile);
#endif
                if (OPT_get (OPTION_DEBUG))
                    write_hunk_debug (n->seg, g_fLoadFile);
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

static bool load_hunk_header(FILE *f)
{
    for (int i=0; i<MAX_NUM_HUNKS; i++)
        g_hunk_table[i] = NULL;
    g_hunk_id_cnt = 0;
    g_hunk_cur = NULL;

    // library names
    uint32_t num_longs;
    if (!fread_u4 (f, &num_longs))
    {
        LOG_printf (LOG_ERROR, "link: read error #26.\n");
        return FALSE;
    }
    if (num_longs)
    {
        // library names are unsupported in this loader
        LOG_printf (LOG_ERROR, "link: read error #27.\n");
        return FALSE;
    }

    uint32_t table_size;
    uint32_t first_hunk_slot;
    uint32_t last_hunk_slot;

    if (!fread_u4 (f, &table_size))
    {
        LOG_printf (LOG_ERROR, "link: read error #28.\n");
        return FALSE;
    }
    if (!fread_u4 (f, &first_hunk_slot))
    {
        LOG_printf (LOG_ERROR, "link: read error #29.\n");
        return FALSE;
    }
    if (!fread_u4 (f, &last_hunk_slot))
    {
        LOG_printf (LOG_ERROR, "link: read error #30.\n");
        return FALSE;
    }

    LOG_printf (LOG_DEBUG, "link: reading hunk header, table_size=%d, first_hunk_slot=%d, last_hunk_slot=%d\n", table_size, first_hunk_slot, last_hunk_slot);

    assert (last_hunk_slot < MAX_NUM_HUNKS);

    for (uint32_t i = first_hunk_slot; i<=last_hunk_slot; i++)
    {
        if (!fread_u4 (f, &g_hunk_sizes[i]))
        {
            LOG_printf (LOG_ERROR, "link: read error #31.\n");
            return FALSE;
        }
    }

    return TRUE;
}

bool LI_segmentListReadLoadFile (U_poolId pid, LI_segmentList sl, string sourcefn, FILE *f)
{
    LOG_printf (LOG_DEBUG, "link: LI_segmentListReadLoadFile: sourcefn=%s\n", sourcefn);

    uint32_t ht;
    if (!fread_u4 (f, &ht))
    {
        LOG_printf (LOG_ERROR, "link: LI_segmentListReadLoadFile: read error #25.\n");
        return FALSE;
    }
    LOG_printf (LOG_DEBUG, "link: LI_segmentListReadLoadFile: %s: hunk type: %08x\n", sourcefn, ht);

    if (ht != HUNK_TYPE_HEADER)
    {
        LOG_printf (LOG_ERROR, "link: LI_segmentListReadLoadFile: %s: this is not a load file, header mismatch: found 0x%08x, expected %08x\n", sourcefn, ht, HUNK_TYPE_HEADER);
        return FALSE;
    }

    for (uint32_t i = 0; i<MAX_NUM_HUNKS; i++)
        g_hunk_sizes[i] = 0;

    if (!load_hunk_header(f))
        return FALSE;

    while (TRUE)
    {
        if (!fread_u4 (f, &ht))
            break;
        LOG_printf (LOG_DEBUG, "link: LI_segmentListReadLoadFile: %s: hunk type: %08x g_hunk_id_cnt=%d\n", sourcefn, ht, g_hunk_id_cnt);

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
                if (!load_hunk_code(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_DATA:
                if (!load_hunk_data(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_BSS:
                if (!load_hunk_bss(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_RELOC32:
                if (!load_hunk_reloc32(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_EXT:
                if (!load_hunk_ext(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_SYMBOL:
                if (!load_hunk_symbol(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_DEBUG:
                if (!load_hunk_debug(pid, sourcefn, f))
                    return FALSE;
                break;
            case HUNK_TYPE_END:
                if (!g_hunk_cur)
                {
                    LOG_printf (LOG_ERROR, "link: LI_segmentListReadLoadFile: hunk_end detected when no hunk was defined.\n");
                    return FALSE;
                }
                LI_segmentListAppend (sl, g_hunk_cur);
                g_hunk_cur = NULL;
                break;
            default:
                LOG_printf (LOG_ERROR, "link: LI_segmentListReadLoadFile: unknown hunk type 0x%08x.\n", ht);
                assert(FALSE);
                return FALSE;
        }
    }
    return TRUE;
}

void LI_relocate (LI_segmentList sl, TAB_table symbols)
{
    for (LI_segmentListNode node = sl->first; node; node=node->next)
    {
        // apply relocs, if any

        if (node->seg->relocs)
        {
            TAB_iter i = TAB_Iter (node->seg->relocs);
            AS_segment seg;
            AS_segmentReloc32 r32;
            while (TAB_next (i, (void **)&seg, (void**) &r32))
            {
                while (r32)
                {
                    assert (r32->offset < node->seg->mem_size);
                    uint32_t *ptr = (uint32_t *)(node->seg->mem + r32->offset);
                    uint32_t ov = *ptr;
                    uint32_t seg_mem = (uint32_t) (uintptr_t) seg->mem;
                    uint32_t nv = ov + seg_mem;
                    //LOG_printf (LOG_DEBUG, "link: LI_relocate: relocating seg (hunk id #%d at 0x%08lx -> #%d at 0x%08lx) at offset %d: 0x%08lx->0x%08lx\n",
                    //            node->seg->hunk_id, node->seg->mem, seg->hunk_id, seg->mem, r32->offset, ov, nv);
                    assert (ov < seg->mem_size);
                    assert(seg_mem);
                    *ptr = nv;
                    r32 = r32->next;
                }
            }
        }

        // relocate symbols

        for (AS_segmentDef def = node->seg->defs; def; def=def->next)
        {
            uint32_t offset = def->offset + (uint32_t) (uintptr_t) node->seg->mem;
            TAB_enter (symbols, def->sym, (void *) (uintptr_t)offset);
            //LOG_printf (LOG_DEBUG, "link: LI_relocate: relocating symbol %s 0x%08lx->0x%08lx\n", S_name (def->sym), def->offset, offset);
        }

        // relocate debug information, if any

        for (AS_srcMapNode n = node->seg->srcMap; n; n=n->next)
        {
            n->offset += (uint32_t) (uintptr_t) node->seg->mem;
        }

        for (AS_frameMapNode n = node->seg->frameMap; n; n=n->next)
        {
            n->code_start += (uint32_t) (uintptr_t) node->seg->mem;
            n->code_end   += (uint32_t) (uintptr_t) node->seg->mem;
        }
    }
}

