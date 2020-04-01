#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "color.h"
#include "flowgraph.h"
#include "liveness.h"
#include "regalloc.h"
#include "table.h"
#include "errormsg.h"

#define ENABLE_DEBUG


#ifdef ENABLE_DEBUG
static Temp_map g_debugTempMap;
#if 0
static void sprintTemp(void* p, string buf)
{
    G_node n = (G_node) p;
    Temp_temp t = (Temp_temp)n->info;
    sprintf(buf, "temp: %s", Temp_look(g_debugTempMap, t));
}
#endif

static void sprintInst(void *p, string buf)
{
    G_node n = (G_node) p;
    AS_instr inst = (AS_instr)n->info;
    AS_sprint(buf, inst, g_debugTempMap);
}
#endif

static AS_instrList reverseInstrList(AS_instrList il)
{
    AS_instrList rl = NULL;
    for (; il; il = il->tail)
    {
        rl = AS_InstrList(il->head, rl);
    }
    return rl;
}

static G_node temp2Node(Temp_temp t, G_graph g)
{
    if (t == NULL)
        return NULL;
    G_nodeList nodes = G_nodes(g);
    G_nodeList p;
    for (p=nodes; p!=NULL; p=p->tail)
    {
        if (Live_gtemp(p->head)==t)
            return p->head;
    }
    return NULL;
}

static Temp_temp node2Temp(G_node n)
{
    if (n == NULL)
        return NULL;
    return Live_gtemp(n);
}

static Temp_tempList L(Temp_temp h, Temp_tempList t)
{
  return Temp_TempList(h, t);
}

static G_node getAlias(G_node n, G_table aliases, Temp_tempList coalescedNodes)
{
    Temp_temp t = node2Temp(n);
    if (Temp_inList(t, coalescedNodes))
    {
        G_node alias = (G_node)G_look(aliases, n);
        return getAlias(alias, aliases, coalescedNodes);
    }
    else
    {
        return n;
    }
}

static Temp_tempList aliased(Temp_tempList tl, G_graph ig,
                             G_table aliases, Temp_tempList cn)
{
    Temp_tempList al = NULL;
    for (; tl; tl = tl->tail)
    {
        Temp_temp t = tl->head;
        G_node n = temp2Node(t, ig);
        assert(n);
        getAlias(n, aliases, cn);
        t = node2Temp(n);
        assert(t);
        al = L(t, al);
    }
    return Temp_union(al, NULL);
};

struct RA_result RA_regAlloc(F_frame f, AS_instrList il)
{
    struct RA_result ret;

    G_graph flow;
    struct Live_graph live;
    struct COL_result col;
    AS_instrList rewriteList;

#ifdef ENABLE_DEBUG
    g_debugTempMap = Temp_layerMap(F_initialRegisters(f), Temp_getNameMap());
#endif

    int try = 0;
    while (++try < 7)
    {
        Temp_map initialRegs = F_initialRegisters(f);
        flow = FG_AssemFlowGraph(il, f);
#ifdef ENABLE_DEBUG
        printf("try #%d flow graph:\n", try);
        printf("-----------------------\n");
        G_show(stdout, G_nodes(flow), sprintInst);
#endif
        live = Live_liveness(flow);
#ifdef ENABLE_DEBUG
        printf("try #%d liveness graph:\n", try);
        printf("-----------------------\n");
        // G_show(stdout, G_nodes(live.graph), sprintTemp);
        Live_showGraph(stdout, G_nodes(live.graph), g_debugTempMap);
#endif
        col = COL_color(live.graph, initialRegs, F_registers(),
                        live.worklistMoves, live.moveList, live.spillCost);

        if (col.spills == NULL)
        {
            break;
        }

        Temp_tempList spilled = col.spills;
        rewriteList = NULL;

        // Assign locals in memory
        Temp_tempList tl;
        TAB_table spilledLocal = TAB_empty();
        for (tl = spilled; tl; tl = tl->tail)
        {
            F_access acc;
            if (f)
            {
                acc = F_allocLocal(f, Temp_ty(tl->head));
            }
            else
            {
                Temp_label name = Temp_namedlabel(strprintf("__spilledtemp_%06d", Temp_num(tl->head)));
                acc = F_allocGlobal(name, Temp_ty(tl->head));
            }

            TAB_enter(spilledLocal, tl->head, acc);
        }

        // Rewrite instructions
        for (; il; il = il->tail)
        {
            AS_instr inst = il->head;
            Temp_tempList useSpilled = Temp_intersect(
                                        aliased(inst->src, live.graph, col.alias, col.coalescedNodes),
                                        spilled);
            Temp_tempList defSpilled = Temp_intersect(
                                        aliased(inst->dst, live.graph, col.alias, col.coalescedNodes),
                                        spilled);
            Temp_tempList tempSpilled = Temp_union(useSpilled, defSpilled);

            // Skip unspilled instructions
            if (tempSpilled == NULL)
            {
                rewriteList = AS_InstrList(inst, rewriteList);
                continue;
            }

            for (tl = useSpilled; tl; tl = tl->tail)
            {
                Temp_temp temp = tl->head;
                F_access local = (F_access)TAB_look(spilledLocal, temp);
                rewriteList = AS_InstrList(                                     // move.l localOffset(fp), temp  /* spilled */
                    AS_InstrEx(AS_MOVE_Ofp_AnDn, AS_w_L, NULL, L(temp, NULL), 0, F_accessOffset(local), NULL), rewriteList);
            }

            rewriteList = AS_InstrList(inst, rewriteList);

            for (tl = defSpilled; tl; tl = tl->tail)
            {
                Temp_temp temp = tl->head;
                F_access local = (F_access)TAB_look(spilledLocal, temp);
                rewriteList = AS_InstrList(                                     // move.l temp, localOffset(FP)  /* spilled */
                    AS_InstrEx(AS_MOVE_AnDn_Ofp, AS_w_L, L(temp, NULL), NULL, 0, F_accessOffset(local), NULL), rewriteList);
            }
        }

        il = reverseInstrList(rewriteList);
    }

    if (col.spills != NULL)
    {
        EM_error(0, "failed to allocate registers");
    }

    //if (col.coalescedMoves != NULL)
    {

        // filter out coalesced moves, NOPs

        rewriteList = NULL;
        Temp_map colTempMap = Temp_layerMap(col.coloring, g_debugTempMap);
        for (; il; il = il->tail)
        {
            AS_instr inst = il->head;

            // skip coalesced moves
            if (AS_instrInList(inst, col.coalescedMoves))
            {
#ifdef ENABLE_DEBUG
                char buf[256];
                AS_sprint(buf, inst, colTempMap);
                printf("/* coalesced: %s */\n", buf);
#endif
                continue;
            }
            else
            {
                // skip NOP
                if (inst->mn == AS_NOP)
                {
#ifdef ENABLE_DEBUG
                    printf("/* NOP */\n");
#endif
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

            rewriteList = AS_InstrList(inst, rewriteList);
        }

        il = reverseInstrList(rewriteList);
    }

    ret.coloring = col.coloring;
    ret.il = il;

#ifdef ENABLE_DEBUG
    printf("register coloring map:\n");
    printf("----------------------\n");
    Temp_dumpMap(stdout, ret.coloring);
#endif

    return ret;
}

