#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "color.h"
#include "flowgraph.h"
#include "liveness.h"
#include "regalloc.h"
#include "table.h"
#include "errormsg.h"
#include "options.h"
#include "linscan.h"

// #define ENABLE_DEBUG

static Temp_temp aliasedSpilled(Temp_temp t, Live_graph g, Temp_tempSet spilled)
{
    if (!Temp_tempSetContains(spilled, t))
        return NULL;
    LG_node n = Live_temp2Node(g, t);
    assert(n);
    n = LG_getAlias(n);
    assert(n);
    return n->temp;
};

/*
 * register allocation by graph coloring
 *
 * this is the full-blown global optimization register allocator
 */

static bool RA_color (F_frame f, AS_instrList il)
{
    FG_graph          flow;
    Live_graph        live;
    struct COL_result col;

    int try = 0;
    while (++try < 7)
    {
        if (OPT_get(OPTION_VERBOSE))
        {
            printf("regalloc try #%d, running before liveness        : ", try);
            U_memstat();
        }

        flow = FG_AssemFlowGraph(il);
#ifdef ENABLE_DEBUG
        printf("try #%d flow graph:\n", try);
        printf("-----------------------\n");
        FG_show(stdout, flow);
#endif

        live = Live_liveness(flow);
#ifdef ENABLE_DEBUG
        printf("try #%d liveness graph:\n", try);
        printf("-----------------------\n");
        Live_showGraph(stdout, live);
#endif

        if (OPT_get(OPTION_VERBOSE))
        {
            printf("regalloc try #%d, after liveness before COL_color: ", try);
            U_memstat();
        }

        col = COL_color(live);

        if (!col.spills)
        {
            break;
        }

        Temp_tempSet spilled = col.spills;

#ifdef ENABLE_DEBUG
        printf("try #%d spilled: %s\n", try, Temp_tempSetSPrint (spilled));
#endif

        // assign memory for spilled temps
        TAB_table spilledLocal = TAB_empty();
        for (Temp_tempSet tn = spilled; tn; tn = tn->tail)
        {
            F_access acc;
            if (f)
            {
                acc = F_allocLocal(f, Temp_ty(tn->temp));
#ifdef ENABLE_DEBUG
                printf("    assigned spilled %s to local fp offset %d\n", Temp_strprint(tn->temp), F_accessOffset(acc));
#endif
            }
            else
            {
                Temp_label l = Temp_namedlabel(strprintf("__spilledtemp_%06d", Temp_num(tn->temp)));
                acc = F_allocGlobal(l, Temp_ty(tn->temp));
#ifdef ENABLE_DEBUG
                printf("    assigned spilled %s to global %s\n", Temp_strprint( tn->temp), Temp_labelstring(l));
#endif
                assert(FALSE); // FIXME: code below doesn't handle globals
            }

            TAB_enter(spilledLocal, tn->temp, acc);
        }

        // modify instruction list, insert move instructions for spilled temps
        AS_instrListNode an = il->first;
        while (an)
        {
            AS_instr inst = an->instr;
            Temp_temp useSpilled = aliasedSpilled(inst->src, live, spilled);
            Temp_temp defSpilled = aliasedSpilled(inst->dst, live, spilled);

            if (useSpilled)
            {
                F_access local = (F_access)TAB_look(spilledLocal, useSpilled);
                AS_instrListInsertBefore (il, an,                                                       // move.x localOffset(fp), useSpilled
                                          AS_InstrEx(AS_MOVE_Ofp_AnDn, AS_tySize(Temp_ty(useSpilled)),
                                                     NULL, useSpilled, 0, F_accessOffset(local), NULL));
            }

            if (defSpilled)
            {
                F_access local = (F_access)TAB_look(spilledLocal, defSpilled);
                AS_instrListInsertAfter (il, an,                                                        // move.x defSpilled, localOffset(FP)
                                         AS_InstrEx(AS_MOVE_AnDn_Ofp, AS_tySize(Temp_ty(defSpilled)),
                                                    defSpilled, NULL, 0, F_accessOffset(local), NULL));
                an = an->next;
            }

            an = an->next;
        }

        /*
         * free memory
         */

        FG_free (flow);

    }

    if (col.spills)
    {
        EM_error(0, "failed to allocate registers");
    }

    if (OPT_get(OPTION_VERBOSE))
    {
        printf("regalloc succeeded:                               ");
        U_memstat();
    }

    /*
     * modify instruction list:
     *
     * - filter out coalesced moves
     * - filter out NOPs
     * - apply register mapping
     */

#ifdef ENABLE_DEBUG
    printf("/* coalesced: moves:\n");
    AS_printInstrSet (stdout, col.coalescedMoves);
    printf("*/\n");
#endif

    AS_instrListNode an = il->first;
    while (an)
    {
        AS_instr inst = an->instr;

        // remove coalesced moves
        if (AS_instrSetContains(col.coalescedMoves, inst))
        {
#ifdef ENABLE_DEBUG
            char buf[256];
            AS_sprint(buf, inst);
            printf("/* coalesced: %s */\n", buf);
#endif
            AS_instrListRemove (il, an);
            an = an->next;
            continue;
        }
        else
        {
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
                // apply register mapping
                if (inst->src)
                {
                    Temp_temp color = TAB_look (col.coloring, inst->src);
                    if (color)
                        inst->src = color;
                }
                if (inst->dst)
                {
                    Temp_temp color = TAB_look (col.coloring, inst->dst);
                    if (color)
                        inst->dst = color;
                }
#ifdef ENABLE_DEBUG
                char buf[256];
                AS_sprint(buf, inst);
                printf("%s\n", buf);
#endif
            }
        }
        an = an->next;
    }

    return TRUE;
}

bool RA_regAlloc(F_frame f, AS_instrList il)
{
    if (OPT_get(OPTION_RACOLOR))
        return RA_color (f, il);

    // register allocation using the linear scan algorithm (Poletto et. al 1999)
    return LS_regalloc (f, il);
}

