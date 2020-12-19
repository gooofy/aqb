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

// #define ENABLE_DEBUG

LG_nodeSet LG_NodeSet(void)
{
    LG_nodeSet nl = (LG_nodeSet) checked_malloc(sizeof *nl);

    nl->first = NULL;
    nl->last  = NULL;

    return nl;
}

bool LG_nodeSetIsEmpty (LG_nodeSet nl)
{
    return nl->first == NULL;
}

static Live_graph Live_Graph(void)
{
    Live_graph lg = (Live_graph) checked_malloc(sizeof *lg);

    lg->nodes                = LG_NodeSet();
    lg->temp2LGNode          = TAB_empty();
    lg->moveWorklist         = AS_InstrSet();

    return lg;
}

static LG_nodeSetNode LG_NodeSetNode (LG_node nn)
{
    LG_nodeSetNode n = checked_malloc(sizeof(*n));

    n->prev  = NULL;
    n->next  = NULL;
    n->node  = nn;

    return n;
}

bool LG_nodeSetAdd (LG_nodeSet nl, LG_node nn)
{
    assert(nl);
    assert(nn);

    if (LG_nodeSetContains (nl, nn))
        return FALSE;

    LG_nodeSetNode n = LG_NodeSetNode(nn);

    n->prev = nl->last;
    if (nl->last)
        nl->last = nl->last->next = n;
    else
        nl->first = nl->last = n;
    return TRUE;
}

bool LG_nodeSetSub (LG_nodeSet nl, LG_node node)
{
    for (LG_nodeSetNode n = nl->first; n; n=n->next)
    {
        if (n->node == node)
        {
            if (n->prev)
            {
                n->prev->next = n->next;
            }
            else
            {
                nl->first = n->next;
                if (n->next)
                    n->next->prev = NULL;
            }

            if (n->next)
            {
                n->next->prev = n->prev;
            }
            else
            {
                nl->last = n->prev;
                if (n->prev)
                    n->prev->next = NULL;
            }

            return TRUE;
        }
    }

    return FALSE;
}

void LG_nodeSetPrint (LG_nodeSet nl)
{
    for (LG_nodeSetNode nln = nl->first; nln; nln=nln->next)
    {
        Temp_temp t = nln->node->temp;
        printf("%-3s", Temp_mapLook(F_registerTempMap(), t));
        if (nln->next)
            printf(", ");
    }
}

bool LG_nodeSetContains (LG_nodeSet nl, LG_node node)
{
    for (LG_nodeSetNode nln = nl->first; nln; nln=nln->next)
    {
        if (nln->node == node)
            return TRUE;
    }
    return FALSE;
}

bool LG_connected (LG_node n1, LG_node n2)
{
  return LG_nodeSetContains(n2->adj, n1);
}

static void LG_addEdge (LG_node n1, LG_node n2)
{
    assert(n1);
    assert(n2);

    LG_nodeSetAdd(n1->adj , n2);
    LG_nodeSetAdd(n2->adj , n1);
}

int LG_computeDegree (LG_node n)
{
    int deg = 0;
    for (LG_nodeSetNode p=n->adj->first; p!=NULL; p=p->next)
        deg++;
    return deg;
}

static inline bool FG_isMove(FG_node n)
{
    AS_instr inst = n->instr;
    return inst->mn == AS_MOVE_AnDn_AnDn;
}

LG_node Live_temp2Node (Live_graph g, Temp_temp t)
{
    assert(t);
    LG_node res = (LG_node) TAB_look(g->temp2LGNode, t);
    assert(res);
    return res;
}

LG_node LG_getAlias(LG_node n)
{
    if (n->alias)
        return LG_getAlias(n->alias);
    return n;
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

static void flowComputeInOut(FG_graph flow)
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

    for (FG_nodeList fl = reverseFlow; fl; fl = fl->tail)
    {
        FG_node n = fl->head;

        Temp_tempSet in = n->in;
        // in[n] ← in[n] ∪ use[n]
        for (Temp_tempSetNode tn=n->use->first; tn; tn = tn->next)
            Temp_tempSetAdd(in, tn->temp);
    }

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
             * in[n] ← in[n] ∪ (out[n] − def[n])
             *
             ********************************************/

            Temp_tempSet in = n->in;
            Temp_tempSet def_n = n->def;
            for (Temp_tempSetNode sn=n->out->first; sn; sn = sn->next)
            {
                Temp_temp t = sn->temp;
                if (Temp_tempSetContains(def_n, t))
                    continue;
                if (Temp_tempSetAdd(in, t))
                    changed = TRUE;
            }

            /********************************************
             *
             * out[n] = ∪<s∈succ[n]> in[s]
             *
             ********************************************/

            Temp_tempSet out = n->out;

            for (FG_nodeList sl = n->succs; sl; sl = sl->tail)
            {
                for (Temp_tempSetNode sn=sl->head->in->first; sn; sn = sn->next)
                {
                    if (Temp_tempSetAdd(out, sn->temp))
                    {
                        changed = TRUE;

#if 0
                        // FIXME: disable debug code
                        if (Temp_num(sn->temp)==19)
                        {
                            char buf1[255], buf2[255];
                            AS_sprint (buf1, n->instr, F_registerTempMap());
                            AS_sprint (buf2, sl->head->instr, F_registerTempMap());
                            printf ("adding temp t%d_ to {out} of %s because it is in the {in} set of %s\n",
                                    Temp_num(sn->temp), buf1, buf2);
                        }
#endif
                    }
                }
            }
        }
#ifdef ENABLE_DEBUG
//        FG_show(stdout, flow, F_registerTempMap());
#endif
    }
}

static LG_node findOrCreateNode(Live_graph lg, Temp_temp t)
{
    LG_node ln = (LG_node) TAB_look (lg->temp2LGNode, t);
    if (ln == NULL)
    {
        ln = (LG_node) checked_malloc(sizeof *ln);

        ln->temp         = t;
        ln->adj          = LG_NodeSet();
        ln->degree       = 0;
        ln->relatedMoves = NULL;
        ln->spillCost    = 0;
        ln->alias        = NULL;
        ln->color        = NULL;

        LG_nodeSetAdd (lg->nodes, ln);
        TAB_enter (lg->temp2LGNode, t, ln);
    }
    return ln;
}

Live_graph Live_liveness(FG_graph flow)
{
    /*
     * compute dataflow in/out sets
     */

    flowComputeInOut(flow);

#ifdef ENABLE_DEBUG
    printf("Live_liveness(): flowComputeInOut result:\n");
    printf("-----------------------------------------\n");
    FG_show(stdout, flow, F_registerTempMap());
#endif

    /*
     * construct interference graph
     */

    Live_graph   lg                   = Live_Graph();

    // traverse flow graph
    for (FG_nodeList fl = flow->nodes; fl; fl = fl->tail)
    {
        FG_node       n    = fl->head;
        AS_instr      inst = n->instr;
        Temp_tempSet  tout = n->out;
        Temp_tempSet  tdef = n->def;
        Temp_tempSet  tuse = n->use;
        Temp_tempSet  defuse = Temp_tempSetUnion(tuse, tdef);
        LG_node       move_src = NULL;

        // Spill Cost
        for (Temp_tempSetNode tn = defuse->first; tn; tn = tn->next)
        {
            Temp_temp ti = tn->temp;
            LG_node n2 = findOrCreateNode(lg, ti);
            n2->spillCost++;
        }

        // Move instruction?
        if (FG_isMove(n))
        {
            move_src = findOrCreateNode(lg, n->instr->src);
            for (Temp_tempSetNode tn = defuse->first; tn; tn = tn->next)
            {
                Temp_temp t = tn->temp;
                LG_node n2 = findOrCreateNode(lg, t);
                if (!n2->relatedMoves)
                    n2->relatedMoves = AS_InstrSet();
                AS_instrSetAdd(n2->relatedMoves, inst);
            }
            AS_instrSetAdd (lg->moveWorklist, inst);
        }

        // traverse defined vars
        for (Temp_tempSetNode t = tdef->first; t; t = t->next) // FIXME: tout ?
        {
            LG_node ndef = findOrCreateNode(lg, t->temp);

            // add edges between output vars and defined var
            for (Temp_tempSetNode tedge = tout->first; tedge; tedge = tedge->next)
            {
                LG_node nedge = findOrCreateNode(lg, tedge->temp);

                // skip if edge is added
                if (ndef == nedge || LG_connected(ndef, nedge))
                    continue;

                // skip src for move instruction
                if (nedge == move_src)
                    continue;

                LG_addEdge(ndef, nedge);
            }
        }

        // add register interference edges for used operands that need to be held in address or data registers only
        if (n->instr)
        {
            if (n->instr->src)
            {
                LG_node nsrc = findOrCreateNode(lg, n->instr->src);
                for (Temp_tempSetNode tn = n->srcInterf->first; tn; tn = tn->next)
                {
                    LG_node interfNode = findOrCreateNode(lg, tn->temp);
                    LG_addEdge(nsrc, interfNode);
                }
            }
            if (n->instr->dst)
            {
                LG_node ndst = findOrCreateNode(lg, n->instr->dst);
                for (Temp_tempSetNode tn = n->dstInterf->first; tn; tn = tn->next)
                {
                    LG_node interfNode = findOrCreateNode(lg, tn->temp);
                    LG_addEdge(ndst, interfNode);
                }
            }
        }
    }

#ifdef ENABLE_DEBUG
    printf("Live_liveness(): interference graph result:\n");
    printf("-------------------------------------------\n");
    Live_showGraph (stdout, lg, F_registerTempMap());
#endif

    return lg;
}

void Live_showGraph(FILE *out, Live_graph lg, Temp_map m)
{
    for (LG_nodeSetNode nln=lg->nodes->first; nln; nln=nln->next)
    {
        LG_node n = nln->node;
        Temp_temp t = n->temp;
        fprintf(out, "%-5s -> ", Temp_mapLook(m, t));

        for (LG_nodeSetNode q = n->adj->first; q; q=q->next)
        {
            LG_node   n2 = q->node;
            Temp_temp r  = n2->temp;
            fprintf(out, " %s", Temp_mapLook(m, r));

            if (q->next)
                fprintf(out, ",");
            else
                fprintf(out, " ");
        }
        fprintf(out, "\n");
    }
}

