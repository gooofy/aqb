#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

// #define ENABLE_DEBUG

Temp_temp Live_gtemp(UG_node n)
{
    return (Temp_temp) n->info;
}

static AS_instrList IL(AS_instr h, AS_instrList t)
{
    return AS_InstrList(h, t);
}

static void enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps)
{
    G_enter(t, flowNode, temps);
}

static Temp_tempList lookupLiveMap(G_table t, G_node flownode)
{
    return (Temp_tempList)G_look(t, flownode);
}

static void getLiveMap(G_graph flow, G_table in, G_table out)
{
    G_table last_in = G_empty();
    G_table last_out = G_empty();
    bool flag = TRUE;

    while (flag)
    {
        for (G_nodeList fl = G_nodes(flow); fl; fl = fl->tail)
        {
            G_node n = fl->head;

            Temp_tempList li = lookupLiveMap(in , n);
            Temp_tempList lo = lookupLiveMap(out, n);

            enterLiveMap(last_in , n, li); // in'[n]  <-- in[n]
            enterLiveMap(last_out, n, lo); // out'[n] <-- out[n]

            Temp_tempList ci = Temp_union(FG_use(n), Temp_minus(lo, FG_def(n)));
            Temp_tempList co = NULL;
            for (G_nodeList sl = G_succ(n); sl; sl = sl->tail)
            {
                co = Temp_union(co, lookupLiveMap(in, sl->head));
            }
            enterLiveMap(in, n, ci);
            enterLiveMap(out, n, co);
        }

        flag = FALSE;
        for (G_nodeList fl = G_nodes(flow); fl; fl = fl->tail)
        {
            G_node n = fl->head;
            Temp_tempList li = lookupLiveMap(in, n);
            Temp_tempList lo = lookupLiveMap(out, n);
            Temp_tempList ci = lookupLiveMap(last_in, n);
            Temp_tempList co = lookupLiveMap(last_out, n);

            if (!Temp_equal(li, ci) || !Temp_equal(lo, co))
            {
                flag = TRUE;
                break;
            }
        }
    }
}

static UG_node findOrCreateNode(Temp_temp t, UG_graph g, TAB_table tab)
{
    UG_node ln = (UG_node)TAB_look(tab, t);
    if (ln == NULL)
    {
        ln = UG_Node(g, t);
        TAB_enter(tab, t, ln);
    }
    return ln;
}

static Live_graph solveLiveness(G_graph flow, G_table in, G_table out)
{
    UG_graph     g             = UG_Graph();
    TAB_table    tab           = TAB_empty();
    Temp_map     moveList      = Temp_empty();
    Temp_map     spillCost     = Temp_empty();
    AS_instrList worklistMoves = NULL;

    // traverse flow graph
    for (G_nodeList fl = G_nodes(flow); fl; fl = fl->tail)
    {
        G_node        n    = fl->head;
        AS_instr      inst = FG_inst(n);
        Temp_tempList tout = lookupLiveMap(out, n);
        Temp_tempList tdef = FG_def(n);
        Temp_tempList tuse = FG_use(n);
        Temp_tempList defuse = Temp_union(tuse, tdef);
        UG_node       move_src = NULL;

        // Spill Cost
        for (Temp_tempList t = defuse; t; t = t->tail)
        {
            Temp_temp ti = t->head;
            long spills = (long)Temp_lookPtr(spillCost, ti);
            ++spills;
            Temp_enterPtr(spillCost, ti, (void*)spills);
        }

        // Move instruction?
        if (FG_isMove(n) && tdef)
        {
            for (; defuse; defuse = defuse->tail)
            {
                Temp_temp t = defuse->head;
                move_src = findOrCreateNode(t, g, tab);
                AS_instrList ml = (AS_instrList)Temp_lookPtr(moveList, t);
                ml = AS_instrUnion(ml, IL(inst, NULL));
                Temp_enterPtr(moveList, t, (void*)ml);
            }

            worklistMoves = AS_instrUnion(worklistMoves, IL(inst, NULL));
        }

        // traverse defined vars
        for (Temp_tempList t = tout; t; t = t->tail)
        {
            UG_node ndef = findOrCreateNode(t->head, g, tab);

            // add edges between output vars and defined var
            for (Temp_tempList tedge = tout; tedge; tedge = tedge->tail)
            {
                UG_node nedge = findOrCreateNode(tedge->head, g, tab);

                // skip if edge is added
                if (ndef == nedge || UG_connected(ndef, nedge))
                {
                    continue;
                }

                // skip src for move instruction
                if (FG_isMove(n) && nedge == move_src)
                    continue;

                UG_addEdge(ndef, nedge);

            }
        }

        // add register interference edges for used operands that need to be held in address or data registers only
        Temp_tempLList tLLIntfRegs = FG_interferingRegsUse(n);
        for (Temp_tempList t = tuse; t; t = t->tail)
        {
            if (!tLLIntfRegs)
                break;
            UG_node nuse = findOrCreateNode(t->head, g, tab);
            for (Temp_tempList interfRegs = tLLIntfRegs->head; interfRegs; interfRegs=interfRegs->tail)
            {
                UG_node interfNode = findOrCreateNode(interfRegs->head, g, tab);
                UG_addEdge(nuse, interfNode);
            }
            tLLIntfRegs = tLLIntfRegs->tail;
        }
        tLLIntfRegs = FG_interferingRegsDef(n);
        for (Temp_tempList t = tdef; t; t = t->tail)
        {
            if (!tLLIntfRegs)
                break;
            UG_node ndef = findOrCreateNode(t->head, g, tab);
            for (Temp_tempList interfRegs = tLLIntfRegs->head; interfRegs; interfRegs=interfRegs->tail)
            {
                UG_node interfNode = findOrCreateNode(interfRegs->head, g, tab);
                UG_addEdge(ndef, interfNode);
            }
            tLLIntfRegs = tLLIntfRegs->tail;
        }
    }

    Live_graph lg = (Live_graph) checked_malloc(sizeof *lg);

    lg->graph = g;
    lg->worklistMoves = worklistMoves;
    lg->moveList = moveList;
    lg->spillCost = spillCost;

    return lg;
}

static G_table g_in, g_out;

#ifdef ENABLE_DEBUG

static void sprintLivemap(void* t, string buf)
{
    char buf2[255];
    G_node n = (G_node) t;
    Temp_tempList li, lo;
    AS_instr inst = (AS_instr) n->info;
    AS_sprint(buf2, inst, Temp_getNameMap());

    int l = strlen(buf2);
    while (l<30)
    {
        buf2[l] = ' ';
        l++;
    }
    buf2[l]=0;

    li = lookupLiveMap(g_in, n);
    lo = lookupLiveMap(g_out, n);

    sprintf(buf, "%s in: %s; out: %s", buf2, Temp_sprint_TempList(li), Temp_sprint_TempList(lo));
}
#endif

Live_graph Live_liveness(G_graph flow)
{
    // Construct liveness graph
    g_in = G_empty(), g_out = G_empty();
    getLiveMap(flow, g_in, g_out);

#ifdef ENABLE_DEBUG
    printf("getLiveMap result:\n");
    printf("------------------\n");
    G_show(stdout, G_nodes(flow), sprintLivemap);
#endif

    // Construct interference graph
    return solveLiveness(flow, g_in, g_out);
}

void Live_showGraph(FILE *out, Live_graph g, Temp_map m)
{
    for (UG_nodeList p=g->graph->nodes; p != NULL; p=p->tail)
    {
        UG_node n = p->head;
        UG_nodeList q;
        assert(n);
        assert(n->info);
        Temp_temp t = (Temp_temp)n->info;
        fprintf(out, "(%2d) %-3s -> ", n->key, Temp_look(m, t));
        for (q=n->adj; q!=NULL; q=q->tail)
        {
            UG_node n2 = q->head;
            assert(n2);
            assert(n2->info);
            Temp_temp r = (Temp_temp)n2->info;
            fprintf(out, " %-3s", Temp_look(m, r));

            if (q->tail)
                fprintf(out, ",");
            else
                fprintf(out, " ");
        }
        fprintf(out, "\n");
    }
}

