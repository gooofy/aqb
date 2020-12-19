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

// #define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
static string tempName(Temp_temp t)
{
    return Temp_mapLook(F_registerTempMap(), t);
}
#endif

#if 0
static UG_node getAlias(UG_node n, UG_table aliases, Temp_tempSet coalescedNodes)
{
    Temp_temp t = node2Temp(n);
    if (Temp_tempSetContains(coalescedNodes, t))
    {
        UG_node alias = (UG_node)UG_look(aliases, n);
        return getAlias(alias, aliases, coalescedNodes);
    }
    else
    {
        return n;
    }
}
#endif

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

struct RA_result RA_regAlloc(F_frame f, AS_instrList il)
{
    struct RA_result  ret = {NULL, NULL};

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

        flow = FG_AssemFlowGraph(il, f);
#ifdef ENABLE_DEBUG
        printf("try #%d flow graph:\n", try);
        printf("-----------------------\n");
        FG_show(stdout, flow, F_registerTempMap());
#endif

        live = Live_liveness(flow);
#ifdef ENABLE_DEBUG
        printf("try #%d liveness graph:\n", try);
        printf("-----------------------\n");
        Live_showGraph(stdout, live, F_registerTempMap());
#endif

        if (OPT_get(OPTION_VERBOSE))
        {
            printf("regalloc try #%d, after liveness before COL_color: ", try);
            U_memstat();
        }

        col = COL_color(live);

        if (Temp_tempSetIsEmpty(col.spills))
        {
            break;
        }

        Temp_tempSet spilled = col.spills;

#ifdef ENABLE_DEBUG
        printf("try #%d spilled: %s\n", try, Temp_tempSetSPrint (spilled, F_registerTempMap()));
#endif

        // assign memory for spilled temps
        TAB_table spilledLocal = TAB_empty();
        for (Temp_tempSetNode tn = spilled->first; tn; tn = tn->next)
        {
            F_access acc;
            if (f)
            {
                acc = F_allocLocal(f, Temp_ty(tn->temp));
#ifdef ENABLE_DEBUG
                printf("    assigned spilled %s to local fp offset %d\n", tempName( tn->temp), F_accessOffset(acc));
#endif
            }
            else
            {
                Temp_label l = Temp_namedlabel(strprintf("__spilledtemp_%06d", Temp_num(tn->temp)));
                acc = F_allocGlobal(l, Temp_ty(tn->temp));
#ifdef ENABLE_DEBUG
                printf("    assigned spilled %s to global %s\n", tempName( tn->temp), Temp_labelstring(l));
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
    }

    if (!Temp_tempSetIsEmpty(col.spills))
    {
        EM_error(0, "failed to allocate registers");
    }

    if (OPT_get(OPTION_VERBOSE))
    {
        printf("regalloc succeeded:                               ");
        U_memstat();
    }

    //if (col.coalescedMoves != NULL)
    {

        // filter out coalesced moves, NOPs

#ifdef ENABLE_DEBUG
        Temp_map colTempMap = Temp_mapLayer(col.coloring, F_registerTempMap());
        printf("/* coalesced: moves:\n");
        AS_printInstrSet (stdout, col.coalescedMoves, colTempMap);
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
                AS_sprint(buf, inst, colTempMap);
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
#ifdef ENABLE_DEBUG
                    char buf[256];
                    AS_sprint(buf, inst, colTempMap);
                    printf("%s\n", buf);
#endif
                }
            }
            an = an->next;
        }
    }

    ret.coloring = col.coloring;
    ret.il = il;

#ifdef ENABLE_DEBUG
    printf("register coloring map:\n");
    printf("----------------------\n");
    Temp_mapDump(stdout, ret.coloring);
#endif
    return ret;
}

