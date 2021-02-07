#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "assem.h"
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

static bool RA_color (CG_frame f, AS_instrList il)
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
            CG_item *item = U_poolAlloc (UP_regalloc, sizeof (*item));
            if (f)
            {
                CG_allocVar (item, f, /*name=*/NULL, /*expt=*/ FALSE, Temp_w(tn->temp) == Temp_w_L ? Ty_ULong() : Ty_UInteger());
#ifdef ENABLE_DEBUG
                printf("    assigned spilled %s to local fp offset %d\n", Temp_strprint(tn->temp), CG_itemOffset(item));
#endif
            }
            else
            {
                string name = strprintf("__spilledtemp_%06d", Temp_num(tn->temp));
                CG_allocVar(item, CG_globalFrame(), name, /*expt=*/ FALSE, Temp_w(tn->temp) == Temp_w_L ? Ty_ULong() : Ty_UInteger());
#ifdef ENABLE_DEBUG
                printf("    assigned spilled %s to global %s\n", Temp_strprint( tn->temp), Temp_labelstring(l));
#endif
                assert(FALSE); // FIXME: code below doesn't handle globals
            }

            TAB_enter(spilledLocal, tn->temp, item);
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
                CG_item *local = (CG_item*) TAB_look(spilledLocal, useSpilled);
                AS_instrListInsertBefore (il, an,                                                       // move.x localOffset(fp), useSpilled
                                          AS_InstrEx(inst->pos, AS_MOVE_Ofp_AnDn, Temp_w(useSpilled),
                                                     NULL, useSpilled, 0, CG_itemOffset(local), NULL));
            }

            if (defSpilled)
            {
                CG_item *local = (CG_item*) TAB_look(spilledLocal, defSpilled);
                AS_instrListInsertAfter (il, an,                                                        // move.x defSpilled, localOffset(FP)
                                         AS_InstrEx(inst->pos, AS_MOVE_AnDn_Ofp, Temp_w(defSpilled),
                                                    defSpilled, NULL, 0, CG_itemOffset(local), NULL));
                an = an->next;
            }

            an = an->next;
        }

        /*
         * free memory
         */

        U_poolReset (UP_flowgraph);

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

bool RA_regAlloc(CG_frame f, AS_instrList il)
{
    if (OPT_get(OPTION_RACOLOR))
        return RA_color (f, il);

    // register allocation using the linear scan algorithm (Poletto et. al 1999)
    return LS_regalloc (f, il);
}

