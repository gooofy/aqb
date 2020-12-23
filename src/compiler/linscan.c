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

#define NUM_DRs 6   // d2-d7

#define ENABLE_DEBUG

typedef struct LS_interv_   *LS_interv;

struct LS_interv_
{
    Temp_temp t;
    int       iStart, iEnd;
    bool      active;
    int       dReg;
};

static TAB_table g_tempIntervals = NULL;    // Temp_temp  -> LS_interv
static TAB_table g_label2idx     = NULL;    // Temp_label -> long

static void swap(LS_interv* a, LS_interv* b) 
{ 
    LS_interv t = *a; 
    *a = *b; 
    *b = t; 
} 

static int partition (LS_interv arr[], int low, int high) 
{ 
    LS_interv pivot = arr[high];
    int i = (low - 1); // Index of smaller element 

    for (int j = low; j <= high- 1; j++) 
    { 
        // If current element is smaller than the pivot 
        if (arr[j]->iStart < pivot->iStart) 
        { 
            i++; // increment index of smaller element 
            swap(&arr[i], &arr[j]); 
        } 
    } 
    swap(&arr[i + 1], &arr[high]); 
    return (i + 1); 
} 

static void quickSort(LS_interv arr[], int low, int high) 
{ 
    if (low < high) 
    { 
        /* pi is partitioning index, arr[p] is now 
        at right place */
        int pi = partition(arr, low, high); 

        // Separately sort elements before 
        // partition and after partition 
        quickSort(arr, low, pi - 1); 
        quickSort(arr, pi + 1, high); 
    } 
} 

static bool updateTempInterval (Temp_temp t, int idx)
{
    if (!t)
        return FALSE;

    LS_interv iv = TAB_look(g_tempIntervals, t);
    if (!iv)
    {
        iv = (LS_interv) checked_malloc(sizeof (*iv));

        iv->t      = t;
        iv->iStart = iv->iEnd = idx;
        iv->active = FALSE;
        iv->dReg   = -1;

        TAB_enter (g_tempIntervals, t, iv);

        return TRUE;
    }

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
        case AS_BCS:
        case AS_BHI:
        case AS_BLS:
        case AS_BCC:
        case AS_JMP:
            return TRUE;
        default:
            return FALSE;
    }
    return FALSE;
}

static void calcLiveIntervals (AS_instrList il)
{
    /*
     * phase 1: determine naive occurence intervals, instr line numbering
     */

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
            updateTempInterval (instr->src, idx);
            updateTempInterval (instr->dst, idx);
        }

        idx++;
    }

    /*
     * phase 2: live interval extension based on jump instructions
     */

    while (TRUE)
    {
        idx=0;
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

                Temp_temp t;
                LS_interv iv;
                TAB_iter i = TAB_Iter(g_tempIntervals);
                while (TAB_next (i, (void **)&t, (void **)&iv))
                {
#ifdef ENABLE_DEBUG
                    // printf ("       interval of %s is %d-%d\n", Temp_strprint(t), iv->iStart, iv->iEnd);
#endif
                    if ((targetIdx < iv->iStart) || (targetIdx > iv->iEnd))
                        continue;
                    bool c = updateTempInterval (t, idx);
#ifdef ENABLE_DEBUG
                    if (c)
                    {
                        char buf[256];
                        AS_sprint (buf, instr);
                        printf ("   extended interval of %s because of %s\n", Temp_strprint(t), buf);
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

#ifdef ENABLE_DEBUG
    {
        printf ("===================================================================\n");
        printf ("temp liveness intervals: \n");

        Temp_temp t;
        LS_interv iv;
        TAB_iter i = TAB_Iter(g_tempIntervals);
        while (TAB_next (i, (void **)&t, (void **)&iv))
        {
            printf ("%-5s: %4d - %4d\n", Temp_strprint (t), iv->iStart, iv->iEnd);
        }

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

static void linearScan (AS_instrList il)
{
    /*
     * sort intervals
     */

    int tmpCnt=0;
    TAB_iter iter = TAB_Iter(g_tempIntervals);
    Temp_temp t;
    LS_interv iv;
    while (TAB_next (iter, (void **)&t, (void **)&iv))
        tmpCnt++;

    LS_interv *ivSorted = checked_malloc(sizeof (LS_interv) * tmpCnt);
    iter = TAB_Iter(g_tempIntervals);
    int idx = 0;
    while (TAB_next (iter, (void **)&t, (void **)&iv))
        ivSorted[idx++] = iv;

    quickSort (ivSorted, 0, tmpCnt-1);

#ifdef ENABLE_DEBUG
    for (int i = 0; i<tmpCnt; i++)
        printf ("LS: sorted iv #%4d: %-5s %d -> %d\n", idx, Temp_strprint(ivSorted[i]->t), ivSorted[i]->iStart, ivSorted[i]->iEnd);
#endif
  
    /*
     * LinearScanRegisterAllocation
     */

    bool dRegs[NUM_DRs];
    for (int i=0; i<NUM_DRs; i++)
        dRegs[i] = FALSE;

    for (int i=0; i<tmpCnt; i++)
    {
        int iStart = ivSorted[i]->iStart;

        // ExpireOldIntervals

        for (int j = 0; j<tmpCnt; j++)
        {
            if ( (!ivSorted[j]->active) || (ivSorted[j]->iEnd >= iStart) )
                continue;
            ivSorted[j]->active = FALSE;
            dRegs[ivSorted[j]->dReg] = FALSE;
        }

        // look for an available register

        int dReg = -1;
        for (int k=0; k<NUM_DRs; k++)
        {
            if (!dRegs[k])
            {
                dRegs[k] = TRUE;
                dReg = k;
                ivSorted[i]->active = TRUE;
                ivSorted[i]->dReg   = dReg;
                break;
            }
        }

        if (dReg < 0)
        {
            //FIXME: spill
            assert(FALSE);
        }
    }

#ifdef ENABLE_DEBUG
    for (int i = 0; i<tmpCnt; i++)
        printf ("LS: dReg for %-5s : %d\n", Temp_strprint(ivSorted[i]->t), ivSorted[i]->dReg);
#endif
}

bool LS_regalloc(F_frame f, AS_instrList il)
{
    g_tempIntervals = TAB_empty();
    g_label2idx     = TAB_empty();

    calcLiveIntervals (il);

    linearScan(il);

    assert(FALSE); // FIXME: implement
    return FALSE;
}

