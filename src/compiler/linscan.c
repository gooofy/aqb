#include <string.h>

#include "linscan.h"
#include "assem.h"

/*
 * register allocation by linear scan
 *
 *
 * references:
 *
 * M. Poletto, V. Sarkar: "Linear Scan Register Allocation", 1999
 * S. Falbesoner: "Implementing a Global Register Allocator for TCC", 2014
 *
 */

//#define ENABLE_DEBUG

/*
 * register mapping (frame register idx -> our register idx)
 *
 * we will not use d0/d1 a0/a1 for coloring since these are callersaves which is not implemented here
 * we will use d4 and a7 during fetch/store of spilled temps
 */

#define NUM_DRs 5   // d2-d6
#define NUM_ARs 2   // a2-a3

                                                 // a0  a1  a2  a3  a4  a6  d0  d1  d2  d3  d4  d5  d6  d7
static int        g_regMapAn [AS_NUM_REGISTERS] = { -1, -1,  0,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  };
static int        g_regMapDn [AS_NUM_REGISTERS] = { -1, -1, -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4, -1  };

/*
 * liveness intervals
 */

typedef struct LS_interv_   *LS_interv;

struct LS_interv_
{
    Temp_temp t;
    int       iStart, iEnd;
    bool      active;
    bool      needsAReg;
    bool      needsDReg;
    int       dReg;
    int       aReg;
    Temp_temp color;    // reg if available
    CG_item   local;    // spilled local otherwise
};

static TAB_table  g_tempIntervals = NULL;    // Temp_temp  -> LS_interv
static TAB_table  g_label2idx     = NULL;    // Temp_label -> long
static int        g_ivCnt;                   // total number of temps / intervals
static LS_interv *g_ivs;                     // intervals sorted by iStart
static LS_interv *g_ive;                     // intervals sorted by iEnd
static TAB_table  g_coloring;                // Temp_temp -> Temp_temp
static TAB_table  g_spilledLocals;           // Temp_temp -> CG_item

/*
 * Quicksort implementation
 */

static void swap(LS_interv* a, LS_interv* b)
{
    LS_interv t = *a;
    *a = *b;
    *b = t;
}

static int partition (LS_interv arr[], int low, int high, bool bSortByStart)
{
    LS_interv pivot = arr[high];
    int i = (low - 1); // Index of smaller element

    for (int j = low; j <= high- 1; j++)
    {
        if (bSortByStart)
        {
            // If current element is smaller than the pivot
            if (arr[j]->iStart < pivot->iStart)
            {
                i++; // increment index of smaller element
                swap(&arr[i], &arr[j]);
            }
        }
        else
        {
            // If current element is smaller than the pivot
            if (arr[j]->iEnd < pivot->iEnd)
            {
                i++; // increment index of smaller element
                swap(&arr[i], &arr[j]);
            }
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

static void quickSort(LS_interv arr[], int low, int high, bool bSortByStart)
{
    if (low < high)
    {
        /* pi is partitioning index, arr[p] is now at right place */
        int pi = partition(arr, low, high, bSortByStart);

        // Separately sort elements before partition and after partition
        quickSort(arr, low   , pi - 1, bSortByStart);
        quickSort(arr, pi + 1, high  , bSortByStart);
    }
}

static bool isJump (enum AS_mn mn)
{
    switch (mn)
    {
        case AS_BEQ:
        case AS_BNE:
        case AS_BLT:
        case AS_BGT:
        case AS_BLE:
        case AS_BGE:
        case AS_BLO:
        case AS_BHI:
        case AS_BLS:
        case AS_BHS:
        case AS_JMP:
            return TRUE;
        default:
            return FALSE;
    }
    return FALSE;
}

static bool updateTempInterval (Temp_temp t, int idx, bool needsAReg, bool needsDReg)
{
    if (!t)
        return FALSE;

    LS_interv iv = TAB_look(g_tempIntervals, t);
    if (!iv)
    {
        iv = (LS_interv) checked_malloc(sizeof (*iv));

        iv->t         = t;
        iv->iStart    = iv->iEnd = idx;
        iv->active    = FALSE;
        iv->dReg      = -1;
        iv->aReg      = -1;
        iv->needsAReg = needsAReg;
        iv->needsDReg = needsDReg;
        iv->color     = NULL;
        CG_NoneItem(&iv->local);

        TAB_enter (g_tempIntervals, t, iv);

        return TRUE;
    }

    iv->needsAReg |= needsAReg;
    iv->needsDReg |= needsDReg;

    bool changed = FALSE;
    if (idx < iv->iStart)
    {
        iv->iStart = idx;
        changed = TRUE;
    }
    if (idx > iv->iEnd)
    {
        iv->iEnd = idx;
        changed = TRUE;
    }
    return changed;
}

static void computeLiveIntervals (AS_instrList il)
{
    /*
     * determine naive occurence intervals, instr line numbering
     */

    {
        int idx = 0;
        for (AS_instrListNode in = il->first; in; in=in->next)
        {
            AS_instr instr = in->instr;

#ifdef ENABLE_DEBUG
            char buf[1024];
            AS_sprint (buf, instr);
            printf ("LS: %4d %s\n", idx, buf);
#endif

            if (instr->mn == AS_LABEL)
            {
                long lIdx = idx;
                TAB_enter (g_label2idx, instr->label, (void *) lIdx);
            }
            else
            {
                updateTempInterval (instr->src, idx, AS_instrInfoA[instr->mn].srcAnOnly,  AS_instrInfoA[instr->mn].srcDnOnly);
                updateTempInterval (instr->dst, idx, AS_instrInfoA[instr->mn].dstAnOnly,  AS_instrInfoA[instr->mn].dstDnOnly);
            }

            idx++;
        }
    }

    /*
     * init interval arrays
     */

    {
        g_ivCnt=0;
        TAB_iter iter = TAB_Iter(g_tempIntervals);
        Temp_temp t;
        LS_interv iv;
        while (TAB_next (iter, (void **)&t, (void **)&iv))
            g_ivCnt++;

        g_ivs = checked_malloc(sizeof (LS_interv) * g_ivCnt);
        g_ive = checked_malloc(sizeof (LS_interv) * g_ivCnt);
        iter = TAB_Iter(g_tempIntervals);
        int i = 0;
        while (TAB_next (iter, (void **)&t, (void **)&iv))
        {
            g_ivs[i] = iv;
            g_ive[i] = iv;
            i++;
        }
    }

    /*
     * live interval extension based on jump instructions
     */

    while (TRUE)
    {
        int  idx     = 0;
        bool changed = FALSE;
        for (AS_instrListNode in = il->first; in; in=in->next)
        {
            AS_instr instr = in->instr;

            if (isJump(instr->mn))
            {
                void *target = TAB_look (g_label2idx, instr->label);
                assert(target);

                long l = (long)target;
                int targetIdx = (int) l;

#ifdef ENABLE_DEBUG
                {
                    //char buf[256];
                    //AS_sprint (buf, instr);
                    //printf ("   target idx of %d %s is %d\n", idx, buf, targetIdx);
                }
#endif

                for (int i=0; i<g_ivCnt; i++)
                {
                    LS_interv iv = g_ivs[i];
#ifdef ENABLE_DEBUG
                    // printf ("       interval of %s is %d-%d\n", Temp_strprint(t), iv->iStart, iv->iEnd);
#endif
                    if ((targetIdx < iv->iStart) || (targetIdx > iv->iEnd))
                        continue;
                    bool c = updateTempInterval (iv->t, idx, FALSE, FALSE);
#ifdef ENABLE_DEBUG
                    if (c)
                    {
                        char buf[256];
                        AS_sprint (buf, instr);
                        printf ("   extended interval of %s because of %s\n", Temp_strprint(iv->t), buf);
                    }
#endif
                    changed |= c;
                }
            }

            idx++;
        }
        if (!changed)
            break;
    }

    /*
     * sort intervals
     */

    quickSort (g_ivs, 0, g_ivCnt-1, /*bSortByStart=*/ TRUE );
    quickSort (g_ive, 0, g_ivCnt-1, /*bSortByStart=*/ FALSE);

#ifdef ENABLE_DEBUG
    {
        printf ("===================================================================\n");
        printf ("temp liveness intervals: \n");

        for (int i = 0; i<g_ivCnt; i++)
            printf ("LS: ivs #%4d: %-5s %d -> %d\n", i, Temp_strprint(g_ivs[i]->t), g_ivs[i]->iStart, g_ivs[i]->iEnd);

        for (int i = 0; i<g_ivCnt; i++)
            printf ("LS: ive #%4d: %-5s %d -> %d\n", i, Temp_strprint(g_ive[i]->t), g_ive[i]->iStart, g_ive[i]->iEnd);

        int idx=0;
        for (AS_instrListNode in = il->first; in; in=in->next)
        {
            AS_instr instr = in->instr;

            char buf[256];
            AS_sprint (buf, instr);
            printf ("LS: %4d %s", idx, buf);

            int pos = 5 + strlen(buf);
            while (pos<50)
            {
                printf (" ");
                pos++;
            }

            Temp_temp t;
            LS_interv iv;
            TAB_iter i = TAB_Iter(g_tempIntervals);
            while (TAB_next (i, (void **)&t, (void **)&iv))
            {
                if ((idx < iv->iStart) || (idx > iv->iEnd))
                    continue;
                printf ("%s ", Temp_strprint(t));
            }
            printf ("\n");
            idx++;
        }
    }
#endif
}

static void spillAtInterval (int i, bool dReg)
{
    // find spill candidate

    int spill=-1;
    for (int j=g_ivCnt-1;j>=0;j--)
    {
        LS_interv iv = g_ive[j];

#ifdef ENABLE_DEBUG
        printf ("LS: looking for spill candidate dReg=%d, j=%d, iv->active=%d, iv->dReg=%d, iv->aReg=%d\n", dReg, j, iv->active, iv->dReg, iv->aReg);
#endif

        if (!iv->active)
            continue;
        if (dReg && iv->dReg>=0)
        {
            spill = j;
            break;
        }
        if (!dReg && iv->aReg>=0)
        {
            spill = j;
            break;
        }
    }

    assert (spill >= 0);

    LS_interv iv      = g_ivs[i];
    LS_interv ivSpill = g_ive[spill];

#ifdef ENABLE_DEBUG
    printf ("LS: spillAtInterval i=%d [%d - %d], spill=%d [%d - %d]\n", i, g_ivs[i]->iStart, g_ivs[i]->iEnd, spill, g_ive[spill]->iStart, g_ive[spill]->iEnd);
#endif

    // spill temp with later iv endpoint

    if (ivSpill->iEnd > iv->iEnd)
    {
        iv->dReg  = ivSpill->dReg ; ivSpill->dReg  =   -1;
        iv->aReg  = ivSpill->aReg ; ivSpill->aReg  =   -1;
        iv->color = ivSpill->color; ivSpill->color = NULL;
        ivSpill->active = FALSE;
        iv->active = TRUE;
    }
    else
    {
        // iv will be spilled
    }

}

static void linearScan (CG_frame f, AS_instrList il)
{
    /*
     * LinearScanRegisterAllocation
     */

    bool dRegs[NUM_DRs];
    for (int i=0; i<NUM_DRs; i++)
        dRegs[i] = FALSE;

    bool aRegs[NUM_ARs];
    for (int i=0; i<NUM_ARs; i++)
        aRegs[i] = FALSE;

    for (int i=0; i<g_ivCnt; i++)
    {
        int iStart  = g_ivs[i]->iStart;
        Temp_temp t = g_ivs[i]->t;

        // handle precolored temps

        if (AS_isPrecolored(t))
        {
            g_ivs[i]->color = t;

            int n = Temp_num(t);
            int ri = g_regMapAn[n];
            if (ri>0)
            {
                assert (!aRegs[ri]); // FIXME: spilling
            }

            ri = g_regMapDn[n];
            if (ri>0)
            {
                assert (!dRegs[ri]); // FIXME: spilling
            }

            continue;
        }

        // ExpireOldIntervals

        for (int j = 0; j<g_ivCnt; j++)
        {
            if ( (!g_ivs[j]->active) || (g_ivs[j]->iEnd >= iStart) )
                continue;
            g_ivs[j]->active = FALSE;
            if (g_ivs[j]->dReg>=0)
            {
                dRegs[g_ivs[j]->dReg] = FALSE;
#ifdef ENABLE_DEBUG
                printf ("LS: released dReg %d from %s (ivs #%d)\n", g_ivs[j]->dReg, Temp_strprint(g_ivs[j]->t), j);
#endif
            }
            if (g_ivs[j]->aReg>=0)
            {
                aRegs[g_ivs[j]->aReg] = FALSE;
#ifdef ENABLE_DEBUG
                printf ("LS: released aReg %d from %s (ivs #%d)\n", g_ivs[j]->aReg, Temp_strprint(g_ivs[j]->t), j);
#endif
            }
        }

        // look for an available register
        if (!g_ivs[i]->needsAReg)
        {
            int dReg = -1;
            for (int k=0; k<NUM_DRs; k++)
            {
                if (!dRegs[k])
                {
                    dRegs[k] = TRUE;
                    dReg = k;
                    g_ivs[i]->active = TRUE;
                    g_ivs[i]->dReg   = dReg;
#ifdef ENABLE_DEBUG
                    printf ("LS: assigned dReg %d to %s (ivs #%d)\n", dReg, Temp_strprint(g_ivs[i]->t), i);
#endif
                    break;
                }
            }

            if (dReg < 0)
                spillAtInterval(i, /*dReg=*/TRUE);
        }
        else
        {
            assert (!g_ivs[i]->needsDReg);

            int aReg = -1;
            for (int k=0; k<NUM_ARs; k++)
            {
                if (!aRegs[k])
                {
                    aRegs[k] = TRUE;
                    aReg = k;
                    g_ivs[i]->active = TRUE;
                    g_ivs[i]->aReg   = aReg;
                    break;
#ifdef ENABLE_DEBUG
                    printf ("LS: assigned aReg %d to %s (ivs #%d)\n", aReg, Temp_strprint(g_ivs[i]->t), i);
#endif
                }
            }

            if (aReg < 0)
                spillAtInterval(i, /*dReg=*/FALSE);
        }
#ifdef ENABLE_DEBUG
        {
            printf ("LS: dRegs=[");
            int allocedDs=0; 
            for (int j=0; j<NUM_DRs;j++)
            {
                printf ("%d ", dRegs[j]);
                if (dRegs[j])
                    allocedDs++;
            }
            printf ("] {");
            int activeDs=0;
            for (int j=0; j<g_ivCnt; j++)
            {
                LS_interv iv = g_ivs[j];
                if (!iv->active || (iv->dReg<0))
                    continue;
                printf ("%s ", Temp_strprint(iv->t));
                activeDs++;
            }
            printf ("}\n");
            assert (allocedDs == activeDs);

            printf ("LS: aRegs=[");
            int allocedAs=0; 
            for (int j=0; j<NUM_ARs;j++)
            {
                printf ("%d ", aRegs[j]);
                if (aRegs[j])
                    allocedAs++;
            }
            printf ("] {");
            int activeAs=0;
            for (int j=0; j<g_ivCnt; j++)
            {
                LS_interv iv = g_ivs[j];
                if (!iv->active || (iv->aReg<0))
                    continue;
                printf ("%s ", Temp_strprint(iv->t));
                activeAs++;
            }
            printf ("}\n");
            assert (allocedAs == activeAs);
        }
#endif
    }

    /*
     * compute color mapping
     */

    Temp_temp dRegSet[NUM_DRs];
    dRegSet[0] = AS_regs[AS_TEMP_D2];
    dRegSet[1] = AS_regs[AS_TEMP_D3];
    dRegSet[2] = AS_regs[AS_TEMP_D4];
    dRegSet[3] = AS_regs[AS_TEMP_D5];
    dRegSet[4] = AS_regs[AS_TEMP_D6];

    Temp_temp aRegSet[NUM_ARs];
    aRegSet[0] = AS_regs[AS_TEMP_A2];
    aRegSet[1] = AS_regs[AS_TEMP_A3];

    g_coloring      = TAB_empty();
    g_spilledLocals = TAB_empty();

    for (int i = 0; i<g_ivCnt; i++)
    {
        Temp_temp color = g_ivs[i]->color;
        Temp_temp t     = g_ivs[i]->t;

        if (!color)
        {
            if (g_ivs[i]->dReg>=0)
            {
                color = dRegSet[g_ivs[i]->dReg];
            }
            else
            {
                if (g_ivs[i]->aReg>=0)
                {
                    color = aRegSet[g_ivs[i]->aReg];
                }
            }
        }
        if (color)
        {
            TAB_enter (g_coloring, t, color);
#ifdef ENABLE_DEBUG
            printf ("LS: reg for %-5s : %s\n", Temp_strprint(t), Temp_strprint(color));
#endif
        }
        else
        {
            CG_item *item = &g_ivs[i]->local;
            CG_allocVar (item, f, /*name=*/NULL, /*expt=*/FALSE, Ty_ULong());
            TAB_enter (g_spilledLocals, t, item);
#ifdef ENABLE_DEBUG
            printf("LS: assigned spilled %s to local fp offset %d\n", Temp_strprint(t), CG_itemOffset(item));
#endif
        }
    }
}

bool LS_regalloc(CG_frame f, AS_instrList il)
{
    g_tempIntervals = TAB_empty();
    g_label2idx     = TAB_empty();

    computeLiveIntervals (il);

    linearScan(f, il);

    /*
     * modify instruction list:
     *
     * - filter out NOPs
     * - apply register mapping
     * - handle spilled temps
     */

    AS_instrListNode an = il->first;
    while (an)
    {
        AS_instr inst = an->instr;

        // remove NOPs
        if (inst->mn == AS_NOP)
        {
#ifdef ENABLE_DEBUG
            printf("/* NOP */\n");
#endif
            AS_instrListRemove (il, an);
            an = an->next;
            continue;
        }
        else
        {
            if (inst->mn == AS_JSR_Label)
            {
                // save all registers in case this was a generic GOSUB
                // (we cannot count on the callee saving registers in that case)

                if (inst->def != AS_callersaves())
                {
                    long regset = (1<<AS_TEMP_A2) | (1<<AS_TEMP_A3) | (1<<AS_TEMP_D2) | (1<<AS_TEMP_D3) | (1<<AS_TEMP_D4) | (1<<AS_TEMP_D5) | (1<<AS_TEMP_D6);
                    AS_instr rsave = AS_InstrEx(inst->pos, AS_MOVEM_Rs_PDsp, Temp_w_L,         // movem.l   a2-a3/d2-d6,-(sp)
                                                NULL, NULL, NULL, regset, NULL);
                    AS_instrListInsertBefore (il, an, rsave);

                    AS_instr rrestore = AS_InstrEx(inst->pos, AS_MOVEM_spPI_Rs, Temp_w_L,      // movem.l   (sp)+, a2-a3/d2-d6
                                                   NULL, NULL, NULL, regset, NULL);
                    AS_instrListInsertAfter (il, an, rrestore);
                    an = an->next;
#ifdef ENABLE_DEBUG
                    char buf[256];
                    AS_sprint(buf, rsave);
                    printf("%s /* save regs before GOSUB */\n", buf);
                    AS_sprint(buf, inst);
                    printf("%s\n", buf);
                    AS_sprint(buf, rrestore);
                    printf("%s /* restore regs after GOSUB */\n", buf);
#endif
                }
            }
            else
            {
                AS_instr spilled_src_move=NULL;
                AS_instr spilled_dst_move=NULL;

                // apply register mapping
                if (inst->src)
                {
                    Temp_temp color = TAB_look (g_coloring, inst->src);
                    if (color)
                    {
                        inst->src = color;
                    }
                    else
                    {
                        // spilled
                        CG_item *item = TAB_look (g_spilledLocals, inst->src);
                        assert(item);
                        Temp_temp r = AS_instrInfoA[inst->mn].srcAnOnly ? AS_regs[AS_TEMP_A0] : AS_regs[AS_TEMP_D0];
                        spilled_src_move = AS_InstrEx(inst->pos, AS_MOVE_Ofp_AnDn, Temp_w_L,                      // move.l localOffset(FP), r
                                                      NULL, r, 0, CG_itemOffset(item), NULL);
                        AS_instrListInsertBefore (il, an, spilled_src_move);
                        inst->src = r;
                    }
                }
                if (inst->dst)
                {
                    Temp_temp color = TAB_look (g_coloring, inst->dst);
                    if (color)
                    {
                        inst->dst = color;
                    }
                    else
                    {
                        CG_item *item = TAB_look (g_spilledLocals, inst->dst);
                        assert(item);
                        Temp_temp r = AS_instrInfoA[inst->mn].dstAnOnly ? AS_regs[AS_TEMP_A1] : AS_regs[AS_TEMP_D1];
                        spilled_dst_move = AS_InstrEx(inst->pos, AS_MOVE_AnDn_Ofp, Temp_w_L,                      // move.l r, localOffset(FP)
                                                      r, NULL, 0, CG_itemOffset(item), NULL);
                        AS_instrListInsertAfter (il, an, spilled_dst_move);
                        inst->dst = r;
                        an = an->next;
                    }
                }
#ifdef ENABLE_DEBUG
                char buf[256];
                if (spilled_src_move)
                {
                    AS_sprint(buf, spilled_src_move);
                    printf("%s /* spilled src */\n", buf);
                }
                AS_sprint(buf, inst);
                printf("%s\n", buf);
                if (spilled_dst_move)
                {
                    AS_sprint(buf, spilled_dst_move);
                    printf("%s /* spilled dst */\n", buf);
                }
#endif
            }
        }
        an = an->next;
    }

    return TRUE;
}

