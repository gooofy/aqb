#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "table.h"
#include "options.h"

/*
#define ENABLE_DEBUG
*/

Temp_temp Live_gtemp(UG_node n)
{
    return (Temp_temp) n->info;
}


static inline bool FG_isMove(FG_node n)
{
    AS_instr inst = n->instr;
    if (!inst)
        return FALSE;
    return inst->mn == AS_MOVE_AnDn_AnDn;
}

#ifdef FG_DEPTH_FIRST_ORDER
static FG_nodeList computeFGDepthFirstOrder (FG_node node, FG_nodeList res)
{
    if (node->mark)
        return res;
    node->mark = TRUE;

    for (FG_nodeList nl = node->preds; nl; nl=nl->tail)
    {
        res = computeFGDepthFirstOrder (nl->head, res);
    }

    res = FG_NodeList (node, res);
    return res;
}
#endif

static void getLiveMap(FG_graph flow)
{
    // optimization: traverse flow graph in reverse order which should reduce the number of iterations needed
#ifdef FG_DEPTH_FIRST_ORDER
    FG_nodeList reverseFlow = computeFGDepthFirstOrder (flow->last_node->head, NULL);
#else
    FG_nodeList reverseFlow = NULL;
    for (FG_nodeList fl = flow->nodes; fl; fl = fl->tail)
    {
        FG_node n = fl->head;
        reverseFlow = FG_NodeList (n, reverseFlow);
    }
#endif

    bool changed = TRUE;
    int nIters = 0;
    while (changed)
    {

        nIters++;
        if (OPT_get(OPTION_VERBOSE))
        {
            printf("liveness iteration #%2d", nIters);
            U_memstat();
        }

        changed = FALSE;
        for (FG_nodeList fl = reverseFlow; fl; fl = fl->tail)
        {
            FG_node n = fl->head;

            /********************************************
             *
             * in[n] ← use[n] ∪ (out[n] − def[n])
             *
             ********************************************/

            Temp_tempSet in = n->in;

            // in[n] ← in[n] ∪ (out[n] − def[n])
            Temp_tempSet def_n = n->def;
            for (Temp_tempSetNode sn=n->out->first; sn; sn = sn->next)
            {
                Temp_temp t = sn->temp;
                if (Temp_tempSetContains(def_n, t))
                    continue;
                if (Temp_tempSetAdd(in, t))
                    changed = TRUE;
            }
            // in[n] ← in[n] ∪ use[n]
            for (Temp_tempSetNode tn=n->use->first; tn; tn = tn->next)
            {
                if (Temp_tempSetAdd(in, tn->temp))
                    changed = TRUE;
            }


            /********************************************
             *
             * out[n] = ∪<s∈succ[n]> ∪in[s]
             *
             ********************************************/

            Temp_tempSet out = n->out;

            for (FG_nodeList sl = n->succs; sl; sl = sl->tail)
            {
                for (Temp_tempSetNode sn=sl->head->in->first; sn; sn = sn->next)
                {
                    if (Temp_tempSetAdd(out, sn->temp))
                        changed = TRUE;
                }
            }
        }
#ifdef ENABLE_DEBUG
        //FG_show(stdout, flow, Temp_layerMap(F_initialRegisters(), Temp_getNameMap()));
#endif
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

static Live_graph solveLiveness(FG_graph flow)
{
    UG_graph     g                    = UG_Graph();
    TAB_table    tab                  = TAB_empty();
    Temp_map     mapTemp2MoveInstrSet = Temp_empty();
    Temp_map     spillCost            = Temp_empty();
    AS_instrSet  worklistMoves        = AS_InstrSet();

    // traverse flow graph
    for (FG_nodeList fl = flow->nodes; fl; fl = fl->tail)
    {
        FG_node       n    = fl->head;
        AS_instr      inst = n->instr;
        Temp_tempSet  tout = n->out;
        Temp_tempSet  tdef = n->def;
        Temp_tempSet  tuse = n->use;
        Temp_tempSet  defuse = Temp_tempSetUnion(tuse, tdef);
        UG_node       move_src = NULL;

        // Spill Cost
        for (Temp_tempSetNode tn = defuse->first; tn; tn = tn->next)
        {
            Temp_temp ti = tn->temp;
            long spills = (long)Temp_lookPtr(spillCost, ti);
            ++spills;
            Temp_enterPtr(spillCost, ti, (void*)spills);
        }

        // Move instruction?
        if (FG_isMove(n) && tdef)
        {
            for (Temp_tempSetNode tn = defuse->first; tn; tn = tn->next)
            {
                Temp_temp t = tn->temp;
                move_src = findOrCreateNode(t, g, tab);
                AS_instrSet ms = (AS_instrSet) Temp_lookPtr(mapTemp2MoveInstrSet, t);
                if (!ms)
                {
                    ms = AS_InstrSet();
                    Temp_enterPtr(mapTemp2MoveInstrSet, t, (void*)ms);
                }
                AS_instrSetAdd(ms, inst);
            }
            AS_instrSetAdd (worklistMoves, inst);
        }

        // traverse defined vars
        for (Temp_tempSetNode t = tout->first; t; t = t->next)
        {
            UG_node ndef = findOrCreateNode(t->temp, g, tab);

            // add edges between output vars and defined var
            for (Temp_tempSetNode tedge = tout->first; tedge; tedge = tedge->next)
            {
                UG_node nedge = findOrCreateNode(tedge->temp, g, tab);

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
        if (n->instr)
        {
            if (n->instr->src)
            {
                UG_node nsrc = findOrCreateNode(n->instr->src, g, tab);
                for (Temp_tempSetNode tn = n->srcInterf->first; tn; tn = tn->next)
                {
                    UG_node interfNode = findOrCreateNode(tn->temp, g, tab);
                    UG_addEdge(nsrc, interfNode);
                }
            }
            if (n->instr->dst)
            {
                UG_node ndst = findOrCreateNode(n->instr->dst, g, tab);
                for (Temp_tempSetNode tn = n->dstInterf->first; tn; tn = tn->next)
                {
                    UG_node interfNode = findOrCreateNode(tn->temp, g, tab);
                    UG_addEdge(ndst, interfNode);
                }
            }
        }
    }

    Live_graph lg = (Live_graph) checked_malloc(sizeof *lg);

    lg->graph = g;
    lg->worklistMoves = worklistMoves;
    lg->mapTemp2MoveInstrSet = mapTemp2MoveInstrSet;
    lg->spillCost = spillCost;

    return lg;
}

Live_graph Live_liveness(FG_graph flow)
{
    // Construct liveness graph
    getLiveMap(flow);

#ifdef ENABLE_DEBUG
    printf("getLiveMap result:\n");
    printf("------------------\n");
    FG_show(stdout, flow, Temp_layerMap(F_initialRegisters(), Temp_getNameMap()));
#endif

    // Construct interference graph
    return solveLiveness(flow);
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
