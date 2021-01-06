#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "assem.h"
#include "liveness.h"
#include "table.h"
#include "options.h"

// #define ENABLE_DEBUG

static Live_graph Live_Graph(void)
{
    Live_graph lg = (Live_graph) checked_malloc(sizeof *lg);

    lg->nodes                = NULL;
    lg->temp2LGNode          = TAB_empty();
    lg->moveWorklist         = AS_InstrSet();

    return lg;
}

LG_nodeList LG_NodeList (LG_node node, LG_nodeList tail)
{
    LG_nodeList nl = (LG_nodeList) checked_malloc(sizeof *nl);

    nl->tail = tail;
    nl->node = node;

    return nl;
}

LG_nodeList LG_nodeListRemove (LG_nodeList nl, LG_node node, bool *bRemoved)
{
    if (!nl)
    {
        *bRemoved = FALSE;
        return NULL;
    }

    if (nl->node == node)
    {
        *bRemoved = TRUE;
        return nl->tail;
    }

    nl->tail = LG_nodeListRemove (nl->tail, node, bRemoved);
    return nl;
}

void LG_nodeListPrint (LG_nodeList nl)
{
    for (; nl; nl=nl->tail)
    {
        Temp_temp t = nl->node->temp;
        char buf[8];
        Temp_snprintf (t, buf, 8);
        printf("%-3s", buf);
        if (nl->tail)
            printf(", ");
    }
}

bool LG_nodeListContains (LG_nodeList nl, LG_node node)
{
    for (; nl; nl=nl->tail)
    {
        if (nl->node == node)
            return TRUE;
    }
    return FALSE;
}

bool LG_connected (LG_node n1, LG_node n2)
{
  return LG_nodeListContains(n2->adj, n1);
}

static void LG_addEdge (LG_node n1, LG_node n2)
{
    assert(n1);
    assert(n2);

#ifdef ENABLE_DEBUG
    assert (!LG_nodeListContains (n1->adj, n2));
    assert (!LG_nodeListContains (n2->adj, n1));
#endif

    n1->adj = LG_NodeList (n2, n1->adj);
    n2->adj = LG_NodeList (n1, n2->adj);
}

int LG_computeDegree (LG_node n)
{
    int deg = 0;
    for (LG_nodeList p=n->adj; p; p=p->tail)
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
    bool bAdded;

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

        // in[n] ← in[n] ∪ use[n]
        for (Temp_tempSet tn=n->use; tn; tn = tn->tail)
            n->in = Temp_tempSetAdd(n->in, tn->temp, &bAdded);
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

            Temp_tempSet def_n = n->def;
            for (Temp_tempSet sn=n->out; sn; sn = sn->tail)
            {
                Temp_temp t = sn->temp;
                if (Temp_tempSetContains(def_n, t))
                    continue;
                n->in = Temp_tempSetAdd(n->in, t, &bAdded);
                changed |= bAdded;
            }

            /********************************************
             *
             * out[n] = ∪<s∈succ[n]> in[s]
             *
             ********************************************/

            for (FG_nodeList sl = n->succs; sl; sl = sl->tail)
            {
                for (Temp_tempSet sn=sl->head->in; sn; sn = sn->tail)
                {
                    n->out = Temp_tempSetAdd(n->out, sn->temp, &bAdded);
                    changed |= bAdded;
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
        ln->adj          = NULL;
        ln->degree       = 0;
        ln->relatedMoves = NULL;
        ln->spillCost    = 0;
        ln->alias        = NULL;
        ln->color        = NULL;

        lg->nodes = LG_NodeList (ln, lg->nodes);
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
    FG_show(stdout, flow);
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
        for (Temp_tempSet tn = defuse; tn; tn = tn->tail)
        {
            Temp_temp ti = tn->temp;
            LG_node n2 = findOrCreateNode(lg, ti);
            n2->spillCost++;
        }

        // Move instruction?
        if (FG_isMove(n))
        {
            move_src = findOrCreateNode(lg, n->instr->src);
            for (Temp_tempSet tn = defuse; tn; tn = tn->tail)
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
        for (Temp_tempSet t = tdef; t; t = t->tail)
        {
            LG_node ndef = findOrCreateNode(lg, t->temp);

            // add edges between output vars and defined var
            for (Temp_tempSet tedge = tout; tedge; tedge = tedge->tail)
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
                for (Temp_tempSet tn = n->srcInterf; tn; tn = tn->tail)
                {
                    LG_node interfNode = findOrCreateNode(lg, tn->temp);
                    if (!LG_connected (nsrc, interfNode))
                        LG_addEdge(nsrc, interfNode);
                }
            }
            if (n->instr->dst)
            {
                LG_node ndst = findOrCreateNode(lg, n->instr->dst);
                for (Temp_tempSet tn = n->dstInterf; tn; tn = tn->tail)
                {
                    LG_node interfNode = findOrCreateNode(lg, tn->temp);
                    if (!LG_connected (ndst, interfNode))
                        LG_addEdge(ndst, interfNode);
                }
            }
        }
        Temp_tempSetFree(defuse);
    }

#ifdef ENABLE_DEBUG
    printf("Live_liveness(): interference graph result:\n");
    printf("-------------------------------------------\n");
    Live_showGraph (stdout, lg);
    Live_stats (lg);
#endif

    return lg;
}

void Live_showGraph(FILE *out, Live_graph lg)
{
    for (LG_nodeList nl=lg->nodes; nl; nl=nl->tail)
    {
        LG_node n = nl->node;
        Temp_temp t = n->temp;
        char buf[8];
        Temp_snprintf (t, buf, 8);
        fprintf(out, "%-5s -> ", buf);

        for (LG_nodeList q = n->adj; q; q=q->tail)
        {
            LG_node   n2 = q->node;
            Temp_temp r  = n2->temp;
            Temp_snprintf (r, buf, 8);
            fprintf(out, " %s", buf);

            if (q->tail)
                fprintf(out, ",");
            else
                fprintf(out, " ");
        }
        fprintf(out, "\n");
    }
}

void Live_stats (Live_graph g)
{
    int n = 0;
    int m = 0;
    for (LG_nodeList nl = g->nodes; nl; nl=nl->tail)
    {
        n++;
        m++;
        for (LG_nodeList a=nl->node->adj;a;a=a->tail)
            m++;
    }
    printf ("Live_stats: # LG_nodes    : %d\n", n);
    printf ("Live_stats: # LG_nodeLists: %d\n", m);
}
