#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "color.h"
#include "liveness.h"
#include "table.h"

#define PRECOLORED_DEGREE   0x7FFFFFFF

// #define ENABLE_DEBUG

static string tempName(Temp_temp t)
{
    return Temp_mapLook(F_registerTempMap(), t);
}

#ifdef ENABLE_DEBUG
static void printts(Temp_tempSet l)
{
    for (Temp_tempSetNode n = l->first; n; n = n->next)
    {
        Temp_temp t = n->temp;
        printf("%-3s", tempName(t));
        if (n->next)
            printf(", ");
    }
}
#endif

typedef struct ctx COL_ctx;

struct ctx
{
    Live_graph    lg;
    Temp_map      precolored;

    Temp_tempSet  regs;
    LG_nodeList    spillWorklist;
    LG_nodeList    freezeWorklist;
    LG_nodeList    simplifyWorklist;
    LG_nodeList    selectStack;

    AS_instrSet   coalescedMoves;
    AS_instrSet   moveWorklist;
    AS_instrSet   activeMoves;

    int           K;
};

static COL_ctx c;

static void addEdge(LG_node nu, LG_node nv)
{
    if (nu == nv) return;
    if (LG_connected(nu, nv)) return;

#ifdef ENABLE_DEBUG
    assert (!LG_nodeListContains (nu->adj, nv));
    assert (!LG_nodeListContains (nv->adj, nu));
#endif
    nu->adj = LG_NodeList (nv, nu->adj);
    nv->adj = LG_NodeList (nu, nv->adj);

    if (nu->degree < PRECOLORED_DEGREE)
        nu->degree++;
    if (nv->degree < PRECOLORED_DEGREE)
        nv->degree++;
}

static bool moveRelated(LG_node n)
{
    AS_instrSet ms = n->relatedMoves;
    if (!ms)
        return FALSE;
    for (AS_instrSetNode n = ms->first; n; n=n->next)
    {
        if (AS_instrSetContains(c.activeMoves, n->instr))
            return TRUE;
        if (AS_instrSetContains(c.moveWorklist, n->instr))
            return TRUE;
    }
    return FALSE;
}

static void enableMove (LG_node n)
{
    AS_instrSet ms = n->relatedMoves;
    if (!ms)
        return;
    for (AS_instrSetNode n = ms->first; n; n=n->next)
    {
        AS_instr instr = n->instr;
        if (AS_instrSetContains(c.activeMoves, instr))
        {
            AS_instrSetSub (c.activeMoves,  instr);
            AS_instrSetAdd (c.moveWorklist, instr);
        }
    }
}

static void removeNode (LG_node n)
{
    for (LG_nodeList nl=n->adj; nl; nl=nl->tail)
    {
        LG_node node = nl->node;

        bool bRemoved;
        node->adj = LG_nodeListRemove (node->adj, n, &bRemoved);
        assert (bRemoved);
        node->degree--;
        assert (node->degree >= 0);

        if (node->degree == (c.K-1))
        {
            c.spillWorklist = LG_nodeListRemove (c.spillWorklist, node, &bRemoved);
            assert(bRemoved);

            if (node->relatedMoves)
            {
                enableMove(node);
#ifdef ENABLE_DEBUG
                assert (!LG_nodeListContains (c.freezeWorklist, node));
#endif
                c.freezeWorklist = LG_NodeList (node, c.freezeWorklist);
            }
            else
            {
#ifdef ENABLE_DEBUG
                assert (!LG_nodeListContains (c.simplifyWorklist, node));
#endif
                c.simplifyWorklist = LG_NodeList (node, c.simplifyWorklist);
            }
        }
    }
}

static void simplify()
{
    LG_node n = c.simplifyWorklist->node;
    c.simplifyWorklist = c.simplifyWorklist->tail;

#ifdef ENABLE_DEBUG
    printf ("simplify(): pushing %s onto selectStack\n", tempName(n->temp));
    assert (!LG_nodeListContains(c.selectStack, n));
#endif

    c.selectStack = LG_NodeList (n, c.selectStack);

    /*
     * decrement degree of all neighbours
     */

    for (LG_nodeList nl=n->adj; nl; nl=nl->tail)
    {
        LG_node node = nl->node;

        node->degree--;
        assert (node->degree >= 0);

        if (node->degree == (c.K-1))
        {
#ifdef ENABLE_DEBUG
            printf ("simplify(): removing neighbour %s (degree: %d) from spillWorklist \n", tempName(node->temp), node->degree);
#endif
            bool bRemoved;
            c.spillWorklist = LG_nodeListRemove (c.spillWorklist, node, &bRemoved);
            assert(bRemoved);

            if (node->relatedMoves)
            {
                enableMove(node);
#ifdef ENABLE_DEBUG
                assert (!LG_nodeListContains (c.freezeWorklist, node));
#endif
                c.freezeWorklist = LG_NodeList(node, c.freezeWorklist);
            }
            else
            {
#ifdef ENABLE_DEBUG
                assert (!LG_nodeListContains (c.simplifyWorklist, node));
#endif
                c.simplifyWorklist = LG_NodeList(node, c.simplifyWorklist);
            }
        }
    }
}

static void combine(LG_node nu, LG_node nv)
{
    if (LG_nodeListContains (c.freezeWorklist, nv))
    {
        bool bRemoved;
        c.freezeWorklist = LG_nodeListRemove (c.freezeWorklist, nv, &bRemoved);
        assert(bRemoved);
#ifdef ENABLE_DEBUG
        printf ("combine() : removed %s from freezeWorklist\n", tempName(nv->temp));
#endif
    }
    else
    {
        bool bRemoved;
        c.spillWorklist = LG_nodeListRemove (c.spillWorklist, nv, &bRemoved);
        assert (bRemoved);
#ifdef ENABLE_DEBUG
        printf ("combine() : removed %s from spillWorklist\n", tempName(nv->temp));
#endif
    }

    assert(!nv->alias);
    nv->alias = nu;

    assert (nv->relatedMoves);
    assert (nu->relatedMoves);
    AS_instrSetAddSet(nu->relatedMoves, nv->relatedMoves);

    enableMove(nv);

    for (LG_nodeList nl=nv->adj; nl; nl=nl->tail)
    {
        addEdge(nl->node, nu);
    }

    removeNode (nv);

    int degree = nu->degree;
#ifdef ENABLE_DEBUG
    printf ("combine() : combined %s with %s, resulting degree: %d (", tempName(nu->temp), tempName(nv->temp), degree);
    LG_nodeListPrint (nu->adj);
    printf (")\n");
#endif
    if (degree >= c.K)
    {
        if (LG_nodeListContains(c.freezeWorklist, nu))
        {
#ifdef ENABLE_DEBUG
            printf ("combine() : moving %s of degree %d from freezeWorklist to spillWorklist\n", tempName(nu->temp), degree);
            assert (!LG_nodeListContains (c.spillWorklist, nu));
#endif
            bool bRemoved;
            c.freezeWorklist = LG_nodeListRemove (c.freezeWorklist, nu, &bRemoved);
            assert(bRemoved);
            c.spillWorklist = LG_NodeList (nu, c.spillWorklist);
        }
    }
    else
    {
        if (LG_nodeListContains(c.spillWorklist, nu))
        {
#ifdef ENABLE_DEBUG
            printf ("combine() : moving %s of degree %d from spillWorklist to freezeWorklist\n", tempName(nu->temp), degree);
            assert (!LG_nodeListContains (c.freezeWorklist, nu));
#endif
            bool bRemoved;
            c.spillWorklist = LG_nodeListRemove (c.spillWorklist, nu, &bRemoved);
            assert(bRemoved);
            c.freezeWorklist = LG_NodeList (nu, c.freezeWorklist);
        }
    }
}

static bool isConstrainedMove(LG_node nu, LG_node nv)
{
    if ( (nv->degree == PRECOLORED_DEGREE) || LG_connected(nu, nv))
        return TRUE;

    return FALSE;
}

static void tryToAddToSimplifyWorklist(LG_node n)
{
    if (!moveRelated(n) && (n->degree < c.K))
    {
        bool bRemoved;
        c.freezeWorklist = LG_nodeListRemove (c.freezeWorklist, n, &bRemoved);
        assert (bRemoved);
#ifdef ENABLE_DEBUG
        assert (!LG_nodeListContains (c.simplifyWorklist, n));
#endif
        c.simplifyWorklist = LG_NodeList (n, c.simplifyWorklist);
    }
}

static void coalesce(void)
{
    AS_instr inst = c.moveWorklist->first->instr;
    LG_node x = Live_temp2Node(c.lg, inst->src);
    LG_node y = Live_temp2Node(c.lg, inst->dst);

    x = LG_getAlias(x);
    y = LG_getAlias(y);

    LG_node u, v;
    if (y->degree > x->degree)
    {
        u = y; v = x;
    }
    else
    {
        u = x; v = y;
    }

#ifdef ENABLE_DEBUG
    {
        char buf[256];
        AS_sprint(buf, inst, F_registerTempMap());
        printf ("coalesce(): considering %s ; => u=%s, v=%s\n", buf,
                tempName(u->temp),
                tempName(v->temp));
    }
#endif

    AS_instrSetSub(c.moveWorklist, inst);

    if (u == v)
    {
        AS_instrSetAdd (c.coalescedMoves, inst);
        tryToAddToSimplifyWorklist(u);
#ifdef ENABLE_DEBUG
        printf ("coalesce():   coalesced.\n");
#endif
    }
    else
    {
        if (isConstrainedMove(u, v))
        {
            tryToAddToSimplifyWorklist(u);
            tryToAddToSimplifyWorklist(v);
#ifdef ENABLE_DEBUG
            printf ("coalesce():   constrained.\n");
#endif
        }
        else
        {
            // Briggs
            int k = 0;
            for (LG_nodeList adj = u->adj; adj; adj=adj->tail)
            {
                if (adj->node->degree >= c.K)
                    k++;
            }
            for (LG_nodeList adj = v->adj; adj; adj=adj->tail)
            {
                if (adj->node->degree >= c.K)
                    k++;
            }

            if (k < c.K)
            {
                AS_instrSetAdd (c.coalescedMoves, inst);
                combine(u, v);
                tryToAddToSimplifyWorklist(u);
#ifdef ENABLE_DEBUG
                printf ("coalesce():   coalesced.\n");
#endif
            }
            else
            {
                AS_instrSetAdd (c.activeMoves, inst);
#ifdef ENABLE_DEBUG
                printf ("coalesce():   active.\n");
#endif
            }
        }
    }
}

static void freezeMoves(LG_node u)
{
    AS_instrSet ms = u->relatedMoves;
    if (!ms)
        return;
    for (AS_instrSetNode n = ms->first; n; n=n->next)
    {
        AS_instr m = n->instr;
        if (!AS_instrSetContains(c.activeMoves, m) && !AS_instrSetContains(c.moveWorklist, m))
            continue;
        AS_instrSetSub (c.activeMoves, m);

        Temp_temp x = n->instr->src;
        Temp_temp y = n->instr->dst;

        LG_node nx = LG_getAlias(Live_temp2Node(c.lg, x));
        if (!moveRelated(nx) && (nx->degree < c.K))
        {
            if (LG_nodeListContains (c.freezeWorklist, nx))
            {
#ifdef ENABLE_DEBUG
                printf("freezeMoves(): moving %s from freezeWorklist to simplifyWorklist\n", tempName(nx->temp));
                assert (!LG_nodeListContains (c.simplifyWorklist, nx));
#endif
                bool bDone;
                c.freezeWorklist = LG_nodeListRemove(c.freezeWorklist, nx, &bDone);
                assert(bDone);
                c.simplifyWorklist = LG_NodeList (nx, c.simplifyWorklist);
            }
        }

        LG_node ny = LG_getAlias(Live_temp2Node(c.lg, y));
        if (ny != nx)
        {
            if (!moveRelated(ny) && (ny->degree < c.K))
            {
                if (LG_nodeListContains (c.freezeWorklist, ny))
                {
#ifdef ENABLE_DEBUG
                    printf("freezeMoves(): moving %s from freezeWorklist to simplifyWorklist\n", tempName(ny->temp));
                    assert (!LG_nodeListContains (c.simplifyWorklist, ny));
#endif
                    bool bDone;
                    c.freezeWorklist = LG_nodeListRemove(c.freezeWorklist, ny, &bDone);
                    assert (bDone);
                    c.simplifyWorklist = LG_NodeList(ny, c.simplifyWorklist);
                }
            }
        }
    }
}

static void freeze()
{
    LG_node u = c.freezeWorklist->node;
    c.freezeWorklist = c.freezeWorklist->tail;

#ifdef ENABLE_DEBUG
    printf("freeze(): moving %s from freezeWorklist to simplifyWorklist\n", tempName(u->temp));
    assert (!LG_nodeListContains (c.simplifyWorklist, u));
#endif

    c.simplifyWorklist = LG_NodeList (u, c.simplifyWorklist);

    freezeMoves(u);
}

static void selectSpill()
{
    float minSpillPriority = 0x7FFFFFFF-1;
    LG_node m = NULL;
    for (LG_nodeList tl = c.spillWorklist; tl; tl = tl->tail)
    {
        LG_node   n      = tl->node;
        int       cost   = n->spillCost;
        int       degree = n->degree;

        assert(degree>=0);

        float priority = ((float)cost) / degree;

        if (priority < minSpillPriority)
        {
            minSpillPriority = priority;
            m = n;
        }
    }

    assert(m);
#ifdef ENABLE_DEBUG
    printf("selectSpill(): potential spill: %s\n", tempName(m->temp));
    assert (!LG_nodeListContains (c.simplifyWorklist, m));
#endif

    bool bDone;
    c.spillWorklist = LG_nodeListRemove (c.spillWorklist, m, &bDone);
    assert (bDone);
    c.simplifyWorklist = LG_NodeList (m, c.simplifyWorklist);

    freezeMoves(m);
}

struct COL_result COL_color(Live_graph lg)
{
    c.lg                   = lg;
    c.regs                 = F_registers();
    c.precolored           = F_initialRegisters();
    c.spillWorklist        = NULL;
    c.freezeWorklist       = NULL;
    c.simplifyWorklist     = NULL;
    c.selectStack          = NULL;
    c.coalescedMoves       = AS_InstrSet();
    c.moveWorklist         = AS_InstrSet(); AS_instrSetAddSet(c.moveWorklist, lg->moveWorklist);
    c.activeMoves          = AS_InstrSet();
    c.K                    = Temp_TempSetCount(c.regs);

#ifdef ENABLE_DEBUG
    Live_showGraph(stdout, c.lg, F_registerTempMap());
    printf("COL_color: c.K=%d\n", c.K);
#endif

    //Temp_tempList coloredNodes = NULL;
    //LG_nodeList   nodes        = live->graph->nodes;

    /*
     * add nodes to worklists
     */

    for (LG_nodeList nl = lg->nodes; nl; nl = nl->tail)
    {
        LG_node   n = nl->node;
        Temp_temp t = n->temp;

        if (Temp_mapLook(c.precolored, t))
        {
            n->degree = PRECOLORED_DEGREE;
            n->color  = t;
#ifdef ENABLE_DEBUG
            printf ("COL_color: %-5s of degree %3d is precolored\n", tempName(t), n->degree);
#endif
            continue;
        }

        n->degree = LG_computeDegree(n);

        if (n->degree >= c.K)
        {
#ifdef ENABLE_DEBUG
            printf ("COL_color: %-5s of degree %3d added to spillWorklist\n", tempName(t), n->degree);
#endif
            c.spillWorklist = LG_NodeList (n, c.spillWorklist);
        }
        else if (moveRelated(n))
        {
            c.freezeWorklist = LG_NodeList (n, c.freezeWorklist);
#ifdef ENABLE_DEBUG
            printf ("COL_color: %-5s of degree %3d added to freezeWorklist\n", tempName(t), n->degree);
#endif
        }
        else
        {
            c.simplifyWorklist = LG_NodeList (n, c.simplifyWorklist);
#ifdef ENABLE_DEBUG
            printf ("COL_color: %-5s of degree %3d added to simplifyWorklist\n", tempName(t), n->degree);
#endif
        }
    }

    /*
     * graph coloring algorithm
     */

    do
    {
//#ifdef ENABLE_DEBUG
//        printf("simplifyWL: "); printts(c.simplifyWorklist); printf("\n");
//        printf("freezeWL  : "); printts(c.freezeWorklist  ); printf("\n");
//        printf("spillWL   : "); printts(c.spillWorklist   ); printf("\n");
//#endif
        if (c.simplifyWorklist)
        {
//#ifdef ENABLE_DEBUG
//            printf("--------------> simplify\n");
//#endif
            simplify();
        }
        else if (!AS_instrSetIsEmpty(c.moveWorklist))
        {
//#ifdef ENABLE_DEBUG
//            printf("--------------> coalesce\n");
//#endif
            coalesce();
        }
        else if (c.freezeWorklist)
        {
//#ifdef ENABLE_DEBUG
//            printf("--------------> freeze\n");
//#endif
            freeze();
        }
        else if (c.spillWorklist)
        {
//#ifdef ENABLE_DEBUG
//            printf("--------------> selectSpill\n");
//#endif
            selectSpill();
        }
    } while (c.simplifyWorklist || !AS_instrSetIsEmpty (c.moveWorklist ) ||
             c.freezeWorklist   || c.spillWorklist                        );

    /*
     * assign colors to non-coalesced temps
     */

    struct COL_result ret = { /* coloring       = */ Temp_mapLayer(Temp_Map(), c.precolored),
                              /* spills         = */ Temp_TempSet(),
                              /* coalescedMoves = */ c.coalescedMoves };

    while (c.selectStack)
    {
        LG_node n = c.selectStack->node; // pop
        c.selectStack = c.selectStack->tail;

#ifdef ENABLE_DEBUG
        printf("colorizing: %s of degree %d.\n", tempName(n->temp), n->degree);
#endif

        Temp_tempSet availableColors = Temp_tempSetCopy(c.regs);

        for (LG_nodeList nl=n->adj; nl; nl=nl->tail)
        {
            LG_node n2 = LG_getAlias(nl->node);
            if (n2->color)
                Temp_tempSetSub (availableColors, n2->color);
        }

#ifdef ENABLE_DEBUG
        printf  ("colorizing:    available colors: ");
        printts (availableColors);
        printf  ("\n");
#endif

        if (Temp_tempSetIsEmpty(availableColors))
        {
#ifdef ENABLE_DEBUG
            printf("colorizing: %s ---> spilled.\n", tempName(n->temp));
#endif
            Temp_tempSetAdd(ret.spills, n->temp);
        }
        else
        {
            n->color = availableColors->first->temp;
            Temp_mapEnter (ret.coloring, n->temp, tempName(n->color));
#ifdef ENABLE_DEBUG
            printf("colorizing: %s ---> %s\n", tempName(n->temp), tempName(n->color));
#endif
        }
    }

    /*
     * copy colors to coalesced nodes from their aliases
    */

    for (LG_nodeList nl = lg->nodes; nl; nl = nl->tail)
    {
        LG_node n = nl->node;
        if (!n->alias)
            continue;
        LG_node a = LG_getAlias(n);
        assert(a);
        if (a->color)
        {
            n->color = a->color;
            Temp_mapEnter (ret.coloring, n->temp, tempName(n->color));
#ifdef ENABLE_DEBUG
            printf("colorizing: %s ---> %s (coalesced)\n", tempName(n->temp), tempName(n->color));
#endif
        }
        else
        {
#ifdef ENABLE_DEBUG
            printf("colorizing: %s ---> no color (coalesced to spilled node)\n", tempName(n->temp));
#endif
        }
    }

    return ret;
}
