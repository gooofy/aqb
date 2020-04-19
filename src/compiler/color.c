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
#include "color.h"
#include "liveness.h"
#include "table.h"

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
static Temp_map g_debugTempMap;

static void printtl(Temp_tempList l)
{
    for (; l; l = l->tail)
    {
        Temp_temp t = l->head;
        printf("(%2d) %-3s", Temp_num(t), Temp_look(g_debugTempMap, t));
        if (l->tail)
            printf(", ");
    }
}

#endif

typedef struct ctx COL_ctx;

struct ctx
{
    UG_graph      lg;
    Temp_map      precolored;
    Temp_tempList initial;
    Temp_tempList regs;
    Temp_tempList spillWorklist;
    Temp_tempList freezeWorklist;
    Temp_tempList simplifyWorklist;
    Temp_tempList spilledNodes;
    Temp_tempList coalescedNodes;
    Temp_tempList coloredNodes;
    Temp_tempList selectStack;

    AS_instrList  coalescedMoves;
    AS_instrList  constrainedMoves;
    AS_instrList  frozenMoves;
    AS_instrList  worklistMoves;
    AS_instrList  activeMoves;

    Temp_map      spillCost;
    Temp_map      moveList;
    UG_table      alias;
    UG_table      degree;

    int           K;
};

static COL_ctx c;

static Temp_tempList instDef(AS_instr inst)
{
    return inst ? inst->dst : NULL;
}

static Temp_tempList instUse(AS_instr inst)
{
    return inst ? inst->src : NULL;
}

static Temp_tempList cloneRegs(Temp_tempList regs)
{
    Temp_tempList tl = NULL;
    for (; regs; regs = regs->tail)
    {
        tl = Temp_TempList(regs->head, tl);
    }
    return tl;
}

static Temp_temp tempHead(Temp_tempList temps)
{
    if (temps == NULL)
        return NULL;
    return temps->head;
}

static UG_node temp2Node(Temp_temp t)
{
    if (t == NULL)
        return NULL;
    for (UG_nodeList p=c.lg->nodes; p!=NULL; p=p->tail)
        if (Live_gtemp(p->head)==t)
            return p->head;
    return NULL;
}

static UG_nodeList tempL2NodeL(Temp_tempList tl)
{
    UG_nodeList nl = NULL;
    for (; tl; tl = tl->tail)
    {
        nl = UG_NodeList(temp2Node(tl->head), nl);
    }
    return UG_reverseNodes(nl);
}

static Temp_temp node2Temp(UG_node n)
{
    if (n == NULL)
        return NULL;
    return Live_gtemp(n);
}

static string tempName(Temp_temp t)
{
    return Temp_look(Temp_getNameMap(), t);
}

static Temp_temp str2Color(string color, Temp_map regcolors, Temp_tempList regs)
{
    for (; regs; regs = regs->tail)
    {
        string s = Temp_look(regcolors, regs->head);
        if (s && (strcmp(s, color) == 0))
        {
            return regs->head;
        }
    }
    //printf("register not found for given color: %s\n", color);
    return NULL;
}

static string color2Str(Temp_temp reg, Temp_map regcolors)
{
    string color = Temp_look(regcolors, reg);
    assert(color);
    return color;
}

static int tempCount(Temp_tempList t)
{
    int cnt = 0;
    for (; t; t = t->tail)
        ++cnt;
    return cnt;
};

static Temp_tempList L(Temp_temp h, Temp_tempList t)
{
    return Temp_TempList(h, t);
}

static AS_instrList IL(AS_instr h, AS_instrList t)
{
    return AS_InstrList(h, t);
}

static Temp_tempList adjacent(Temp_temp t)
{
    UG_node n = temp2Node(t);
    Temp_tempList adjs = NULL;
    for (UG_nodeList adjn = n->adj; adjn; adjn = adjn->tail)
    {
        // FIXME: wrong, remove ?! adjs = L(node2Temp(n), adjs);
        adjs = L(node2Temp(adjn->head), adjs);
    }

    adjs = Temp_minus(adjs, Temp_union(c.selectStack, c.coalescedNodes));
    return adjs;
}

static void addEdge(UG_node nu, UG_node nv)
{
    if (nu == nv) return;
    if (UG_connected(nu, nv)) return;
    UG_addEdge(nu, nv);

    Temp_temp u = node2Temp(nu);
    Temp_temp v = node2Temp(nv);

    if (Temp_look(c.precolored, u) == NULL)
    {
        long d = (long)UG_look(c.degree, nu);
        d += 1;
        UG_enter(c.degree, nu, (void*)d);
    }

    if (Temp_look(c.precolored, v) == NULL)
    {
        long d = (long)UG_look(c.degree, nv);
        d += 1;
        UG_enter(c.degree, nv, (void*)d);
    }
}

static AS_instrList nodeMoves(Temp_temp t)
{
    AS_instrList ml = (AS_instrList)Temp_lookPtr(c.moveList, t);
    return AS_instrIntersect(ml, AS_instrUnion(c.activeMoves, c.worklistMoves));
}

static bool moveRelated(Temp_temp t)
{
    return nodeMoves(t) != NULL;
}

static void makeWorkList()
{
    Temp_tempList tl;
    for (tl = c.initial; tl; tl = tl->tail)
    {
        Temp_temp t = tl->head;
        UG_node n = temp2Node(t);
        c.initial = Temp_minus(c.initial, L(t, NULL));

        if (UG_degree(n) >= c.K)
        {
            c.spillWorklist = Temp_union(c.spillWorklist, L(t, NULL));
        }
        else if (moveRelated(t))
        {
            c.freezeWorklist = Temp_union(c.freezeWorklist, L(t, NULL));
        }
        else
        {
            c.simplifyWorklist = Temp_union(c.simplifyWorklist, L(t, NULL));
        }
    }
}

static void enableMoves(Temp_tempList tl)
{
    for (; tl; tl = tl->tail)
    {
        AS_instrList il = nodeMoves(tl->head);
        for (; il; il = il->tail)
        {
            AS_instr m = il->head;
            if (AS_instrInList(m, c.activeMoves))
            {
                c.activeMoves = AS_instrMinus(c.activeMoves, IL(m, NULL));
                c.worklistMoves = AS_instrUnion(c.worklistMoves, IL(m, NULL));
            }
        }
    }
}

static void decrementDegree(UG_node n)
{
    Temp_temp t = node2Temp(n);
    long d = (long)UG_look(c.degree, n);
    d -= 1;
    UG_enter(c.degree, n, (void*)d);

    if (d == c.K)
    {
        enableMoves(L(t, adjacent(t)));
        c.spillWorklist = Temp_minus(c.spillWorklist, L(t, NULL));
        if (moveRelated(t))
        {
            c.freezeWorklist = Temp_union(c.freezeWorklist, L(t, NULL));
        }
        else
        {
            c.simplifyWorklist = Temp_union(c.simplifyWorklist, L(t, NULL));
        }
    }
}

static void addWorkList(Temp_temp t)
{
    long degree = (long)UG_look(c.degree, temp2Node(t));
    if (Temp_look(c.precolored, t) == NULL &&
        (!moveRelated(t)) &&
        (degree < c.K))
    {
        c.freezeWorklist   = Temp_minus(c.freezeWorklist, L(t, NULL));
        c.simplifyWorklist = Temp_union(c.simplifyWorklist, L(t, NULL));
    }
}

static bool OK(Temp_temp t, Temp_temp r)
{
    UG_node nt = temp2Node(t);
    UG_node nr = temp2Node(r);
    long degree = (long)UG_look(c.degree, nt);
    if (degree < c.K)
    {
        return TRUE;
    }
    if (Temp_look(c.precolored, t))
    {
        return TRUE;
    }
    if (UG_connected(nt, nr))
    {
        return TRUE;
    }
    return FALSE;
}

static bool isCoalescingConservativeBriggs(Temp_tempList tl)
{
    UG_nodeList nl = tempL2NodeL(tl);
    int k = 0;
    for (; nl; nl = nl->tail)
    {
        long degree = (long)UG_look(c.degree, nl->head);
        if (degree >= c.K)
        {
            ++k;
        }
    }
    return (k < c.K);
}

static UG_node getAlias(UG_node n)
{
    Temp_temp t = node2Temp(n);
    if (Temp_inList(t, c.coalescedNodes))
    {
        UG_node alias = (UG_node)UG_look(c.alias, n);
        return getAlias(alias);
    }
    else
    {
        return n;
    }
}

static void simplify()
{
    if (c.simplifyWorklist == NULL)
    {
        return;
    }

    Temp_temp t = c.simplifyWorklist->head;
    UG_node n = temp2Node(t);
    c.simplifyWorklist = c.simplifyWorklist->tail;

    c.selectStack = L(t, c.selectStack);  // push
#ifdef ENABLE_DEBUG
    printf ("simplify(): pushed %d\n", Temp_num(t));
#endif

    UG_nodeList adjs = n->adj;
    for (; adjs; adjs = adjs->tail)
    {
        UG_node m = adjs->head;
        decrementDegree(m);
    }
}

static void combine(Temp_temp u, Temp_temp v)
{
    UG_node nu = temp2Node(u);
    UG_node nv = temp2Node(v);
    if (Temp_inList(v, c.freezeWorklist))
    {
        c.freezeWorklist = Temp_minus(c.freezeWorklist, L(v, NULL));
    }
    else
    {
        c.spillWorklist = Temp_minus(c.spillWorklist, L(v, NULL));
    }

    c.coalescedNodes = Temp_union(c.coalescedNodes, L(v, NULL));
    UG_enter(c.alias, nv, (void*)nu);

    AS_instrList au = (AS_instrList)Temp_lookPtr(c.moveList, u);
    AS_instrList av = (AS_instrList)Temp_lookPtr(c.moveList, v);
    au = AS_instrUnion(au, av);
    Temp_enterPtr(c.moveList, u, (void*)au);

    enableMoves(L(v, NULL));

    UG_nodeList adjs = nv->adj;
    for (; adjs; adjs = adjs->tail)
    {
        UG_node nt = adjs->head;
        nt = getAlias(nt);
        addEdge(nt, nu);
        decrementDegree(nt);
    }

    long degree = (long)UG_look(c.degree, nu);
    printf ("combined %d with %d, resulting degree: %ld\n", Temp_num(u), Temp_num(v), degree);
    if (degree >= c.K && Temp_inList(u, c.freezeWorklist))
    {
        c.freezeWorklist = Temp_minus(c.freezeWorklist, L(u, NULL));
        c.spillWorklist = Temp_union(c.spillWorklist, L(u, NULL));
    }
}

static bool isConstrainedMove(Temp_temp u, Temp_temp v)
{
    UG_node nu = temp2Node(u);
    UG_node nv = temp2Node(v);

    if (Temp_look(c.precolored, v) || UG_connected(nu, nv))
        return TRUE;

    return FALSE;

#if 0
    // resulting node still colorable (at least one register needs to exist

    Temp_tempList adju = adjacent(u);
    Temp_tempList adjv = adjacent(v);
    Temp_tempList adj = Temp_union(adju, adjv);

#ifdef ENABLE_DEBUG
    printf("adju: "); printtl(adju); printf("\n");
    printf("adjv: "); printtl(adjv); printf("\n");
    printf("adj : "); printtl(adj);  printf("\n");
#endif
    for (Temp_tempList r=c.regs; r; r=r->tail)
    {
        if (!Temp_inList(r->head, adj))
            return FALSE;
    }
    return TRUE;
#endif
}

static void coalesce()
{
    assert(c.worklistMoves);

    AS_instr inst = c.worklistMoves->head;
    Temp_temp x = tempHead(instUse(inst));
    Temp_temp y = tempHead(instDef(inst));

    x = node2Temp(getAlias(temp2Node(x)));
    y = node2Temp(getAlias(temp2Node(y)));

    Temp_temp u, v;
    if (Temp_look(c.precolored, y) != NULL)
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
        AS_sprint(buf, inst, g_debugTempMap);
        printf ("attempting move coalescing for: %s x=%s, y=%s, u=%s, v=%s\n", buf,
                Temp_look(g_debugTempMap, x),
                Temp_look(g_debugTempMap, y),
                Temp_look(g_debugTempMap, u),
                Temp_look(g_debugTempMap, v));
    }
#endif

    c.worklistMoves = AS_instrMinus(c.worklistMoves, IL(inst, NULL));

    if (u == v)
    {
        c.coalescedMoves = AS_instrUnion(c.coalescedMoves, IL(inst, NULL));
        addWorkList(u);
#ifdef ENABLE_DEBUG
        printf ("   coalesced.\n");
#endif
    }
    else
    {
        if (isConstrainedMove(u, v))
        {
            c.constrainedMoves = AS_instrUnion(c.constrainedMoves, IL(inst, NULL));
            addWorkList(u);
            addWorkList(v);
#ifdef ENABLE_DEBUG
            printf ("   constrained.\n");
#endif
        }
        else
        {
            bool flag = FALSE;
            if (Temp_look(c.precolored, u))
            {
                flag = TRUE;
                Temp_tempList adj = adjacent(v);
                for (; adj; adj = adj->tail)
                {
                    if (!OK(adj->head, u))
                    {
                        flag = FALSE;
                        break;
                    }
                }
            }
            else
            {
                Temp_tempList adju = adjacent(u);
                Temp_tempList adjv = adjacent(v);
                Temp_tempList adj = Temp_union(adju, adjv);
                flag = isCoalescingConservativeBriggs(adj);
            }

            if (flag)
            {
                c.coalescedMoves = AS_instrUnion(c.coalescedMoves, IL(inst, NULL));
                combine(u, v);
                addWorkList(u);
#ifdef ENABLE_DEBUG
                printf ("   coalesced.\n");
#endif
            }
            else
            {
                c.activeMoves = AS_instrUnion(c.activeMoves, IL(inst, NULL));
#ifdef ENABLE_DEBUG
                printf ("   active.\n");
#endif
            }
        }
    }
}

static void freezeMoves(Temp_temp u)
{
    AS_instrList il = nodeMoves(u);
    for (; il; il = il->tail)
    {
        AS_instr m = il->head;
        Temp_temp x = tempHead(instUse(m));
        Temp_temp y = tempHead(instDef(m));
        UG_node nx = temp2Node(x);
        UG_node ny = temp2Node(y);
        UG_node nv;

        if (getAlias(nx) == getAlias(ny))
        {
            nv = getAlias(nx);
        }
        else
        {
            nv = getAlias(ny);
        }
        Temp_temp v = node2Temp(nv);

        c.activeMoves = AS_instrMinus(c.activeMoves, IL(m, NULL));
        c.frozenMoves = AS_instrUnion(c.frozenMoves, IL(m, NULL));

        long degree = (long)UG_look(c.degree, nv);
        if (nodeMoves(v) == NULL && degree < c.K)
        {
            c.freezeWorklist = Temp_minus(c.freezeWorklist, L(v, NULL));
            c.simplifyWorklist = Temp_union(c.simplifyWorklist, L(v, NULL));
        }
    }
}

static void freeze() {
  if (c.freezeWorklist == NULL) {
    return;
  }

  Temp_temp u = c.freezeWorklist->head;
  c.freezeWorklist = Temp_minus(c.freezeWorklist, L(u, NULL));
  c.simplifyWorklist = Temp_union(c.simplifyWorklist, L(u, NULL));
  freezeMoves(u);
}

static void selectSpill()
{
    if (c.spillWorklist == NULL)
    {
        return;
    }
    Temp_tempList tl = c.spillWorklist;
    float minSpillPriority = 9999.0f;
    Temp_temp m = NULL;
    for (; tl; tl = tl->tail)
    {
        Temp_temp t = tl->head;
        long cost = (long)Temp_lookPtr(c.spillCost, t);
        long degree = (long)UG_look(c.degree, temp2Node(t));
        degree = (degree > 0) ? degree : 1;
        float priority = ((float)cost) / degree;
        if (priority < minSpillPriority)
        {
            minSpillPriority = priority;
            m = t;
        }
    }
    c.spillWorklist    = Temp_minus(c.spillWorklist,    L(m, NULL));
    c.simplifyWorklist = Temp_union(c.simplifyWorklist, L(m, NULL));
    freezeMoves(m);
}

struct COL_result COL_color(Live_graph live, Temp_map initial, Temp_tempList regs)
{
    struct COL_result ret = { NULL, NULL, NULL };

    //    col = COL_color(live->graph, initialRegs, F_registers(),
    //                    live->worklistMoves, live->moveList, live->spillCost);

    c.precolored       = initial;
    c.regs             = regs;
    c.initial          = NULL;
    c.simplifyWorklist = NULL;
    c.freezeWorklist   = NULL;
    c.spillWorklist    = NULL;
    c.spilledNodes     = NULL;
    c.coalescedNodes   = NULL;
    c.coloredNodes     = NULL;
    c.selectStack      = NULL;

    c.coalescedMoves   = NULL;
    c.constrainedMoves = NULL;
    c.frozenMoves      = NULL;
    c.worklistMoves    = live->worklistMoves;
    c.activeMoves      = NULL;

    c.spillCost        = live->spillCost;
    c.moveList         = live->moveList;
    c.degree           = UG_empty();
    c.alias            = UG_empty();
    c.lg               = live->graph;

    c.K                = tempCount(regs);

    Temp_map      precolored   = initial;
    Temp_map      colors       = Temp_layerMap(Temp_empty(), initial);
    Temp_tempList coloredNodes = NULL;
    UG_nodeList   nodes        = live->graph->nodes;

#ifdef ENABLE_DEBUG
    g_debugTempMap = Temp_layerMap(initial, Temp_getNameMap());
#endif

    for (UG_nodeList nl = nodes; nl; nl = nl->tail)
    {
        long degree = UG_degree(nl->head);
        UG_enter(c.degree, nl->head, (void*)degree);
        Temp_temp t = node2Temp(nl->head);

        if (Temp_look(precolored, t))
        {
            UG_enter(c.degree, nl->head, (void*)999);
            continue;
        }
        c.initial = L(t, c.initial);
    }

    // main coloring algorithm
    makeWorkList();
    do
    {
#ifdef ENABLE_DEBUG
        printf("simplifyWL: "); printtl(c.simplifyWorklist); printf("\n");
        printf("freezeWL  : "); printtl(c.freezeWorklist  ); printf("\n");
        printf("spillWL   : "); printtl(c.spillWorklist   ); printf("\n");
#endif
        if (c.simplifyWorklist != NULL)
        {
#ifdef ENABLE_DEBUG
            printf("--------------> simplify\n");
#endif
            simplify();
        }
        else if (c.worklistMoves != NULL)
        {
#ifdef ENABLE_DEBUG
            printf("--------------> coalesce\n");
#endif
            coalesce();
        }
        else if (c.freezeWorklist != NULL)
        {
#ifdef ENABLE_DEBUG
            printf("--------------> freeze\n");
#endif
            freeze();
        }
        else if (c.spillWorklist != NULL)
        {
#ifdef ENABLE_DEBUG
            printf("--------------> selectSpill\n");
#endif
            selectSpill();
        }
#ifdef ENABLE_DEBUG
        Live_showGraph(stdout, live, g_debugTempMap);
#endif
    } while (c.simplifyWorklist != NULL || c.worklistMoves != NULL ||
             c.freezeWorklist != NULL   || c.spillWorklist != NULL);

    // for (nl = nodes; nl; nl = nl->tail) {
    //   if (Temp_look(precolored, node2Temp(nl->head))) {
    //     continue;
    //   }
    //   c.selectStack = L(node2Temp(nl->head), c.selectStack);
    // }

    while (c.selectStack != NULL)
    {
        Temp_temp t = c.selectStack->head; // pop
        UG_node n = temp2Node(t);
        c.selectStack = c.selectStack->tail;

        Temp_tempList okColors = cloneRegs(regs);
        UG_nodeList adjs = n->adj;

        for (; adjs; adjs = adjs->tail)
        {
            UG_node nw = adjs->head;
            UG_node nw_alias = getAlias(nw);
            Temp_temp w_alias = node2Temp(nw_alias);
            string color;
            if ((color = Temp_look(colors, w_alias)) != NULL)
            {
                Temp_temp colorTemp = str2Color(color, precolored, regs);
                if (colorTemp)
                {
                    okColors = Temp_minus(okColors, L(colorTemp, NULL));
                }
            }
        }

        if (okColors == NULL)
        {
            c.spilledNodes = L(t, c.spilledNodes);
        }
        else
        {
            coloredNodes = L(t, coloredNodes);
            Temp_enter(colors, t, color2Str(okColors->head, precolored));
        }
    }

    Temp_tempList tl;
    for (tl = c.coalescedNodes; tl; tl = tl->tail)
    {
        UG_node alias = getAlias(temp2Node(tl->head));
        string color = Temp_look(colors, node2Temp(alias));
        Temp_enter(colors, tl->head, color);
    }

    ret.coloring = colors;

    ret.colored = NULL;
    for (; coloredNodes; coloredNodes = coloredNodes->tail)
    {
        ret.colored = L(coloredNodes->head, ret.colored);
    }

    ret.spills = NULL;
    for (; c.spilledNodes; c.spilledNodes = c.spilledNodes->tail)
    {
        printf("spilled: %s\n", tempName(c.spilledNodes->head));
        ret.spills = L(c.spilledNodes->head, ret.spills);
    }

    ret.coalescedMoves = c.coalescedMoves;
    ret.coalescedNodes = c.coalescedNodes;
    ret.alias = c.alias;

    return ret;
}


