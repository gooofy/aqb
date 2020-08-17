/*
 * graph.c - Functions to manipulate and create control flow and
 *           interference graphs.
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "errormsg.h"
#include "table.h"

G_graph G_Graph(void)
{
    G_graph g = (G_graph) checked_malloc(sizeof *g);

    g->nodecount = 0;
    g->mynodes   = NULL;
    g->mylast    = NULL;

    return g;
}

G_nodeList G_NodeList(G_node head, G_nodeList tail)
{
    G_nodeList n = (G_nodeList) checked_malloc(sizeof *n);

    n->head = head;
    n->tail = tail;

    return n;
}

/* generic creation of G_node */
G_node G_Node(G_graph g, void *info)
{
    G_node n = (G_node)checked_malloc(sizeof *n);

    G_nodeList p = G_NodeList(n, NULL);

    assert(g);
    n->mygraph = g;
    n->mykey   = g->nodecount++;

    if (g->mylast==NULL)
        g->mynodes = g->mylast = p;
    else
        g->mylast  = g->mylast->tail = p;

    n->succs = NULL;
    n->preds = NULL;
    n->info  = info;

    return n;
}

G_nodeList G_nodes(G_graph g)
{
    assert(g);
    return g->mynodes;
}

G_nodeList G_reverseNodes(G_nodeList l)
{
    G_nodeList nl = NULL;
    for (; l; l = l->tail)
        nl = G_NodeList(l->head, nl);
    return nl;
}

/* return true if a is in l list */
bool G_inNodeList(G_node a, G_nodeList l)
{
    G_nodeList p;
    for(p=l; p!=NULL; p=p->tail)
        if (p->head==a)
            return TRUE;
    return FALSE;
}

void G_addEdge(G_node from, G_node to)
{
    assert(from);
    assert(to);
    assert(from->mygraph == to->mygraph);

    if (G_goesTo(from, to))
        return;
    to->preds   = G_NodeList(from, to->preds);
    from->succs = G_NodeList(to, from->succs);
}

static G_nodeList delete(G_node a, G_nodeList l)
{
    assert(a && l);
    if (a==l->head)
        return l->tail;
    else
        return G_NodeList(l->head, delete(a, l->tail));
}

void G_rmEdge(G_node from, G_node to)
{
    assert(from && to);
    to->preds   = delete(from, to->preds);
    from->succs = delete(to, from->succs);
}

 /**
  * Print a human-readable dump for debugging.
  */
void G_show(FILE *out, G_nodeList p, void showInfo(void *, string buf))
{
    for (; p!=NULL; p=p->tail)
    {
        char buf[255];
        int  cnt=0;

        G_node n = p->head;
        G_nodeList q;
        assert(n);
        fprintf(out, " (%3d) -> ", n->mykey);
        for (q=G_succ(n); q!=NULL; q=q->tail)
        {
            fprintf(out, "%3d", q->head->mykey);
            if (q->tail)
                fprintf(out, ",");
            else
                fprintf(out, " ");
            cnt++;
        }
        for (;cnt<3;cnt++)
            fprintf(out, "    ");
        if (n->info)
        {
            showInfo(n, buf);
            fprintf(out, "%s\n", buf);
        }
        else
        {
            fprintf(out, "NIL\n");
        }
    }
}

G_nodeList G_succ(G_node n) { assert(n); return n->succs; }

G_nodeList G_pred(G_node n) { assert(n); return n->preds; }

bool G_goesTo(G_node from, G_node n) {
  return G_inNodeList(n, G_succ(from));
}

/* return length of predecessor list for node n */
static int inDegree(G_node n)
{
    int deg = 0;
    G_nodeList p;
    for(p=G_pred(n); p!=NULL; p=p->tail)
    {
        //printf("in: %d", p->head->mykey);
        deg++;
    }
    return deg;
}

/* return length of successor list for node n */
static int outDegree(G_node n)
{
    int deg = 0;
    G_nodeList p;
    for(p=G_succ(n); p!=NULL; p=p->tail)
    {
        //printf("out: %d", p->head->mykey);
        deg++;
    }
    return deg;
}

int G_degree(G_node n)
{
    int d = inDegree(n)+outDegree(n);
    //printf("\n");
    return d;
}

/* put list b at the back of list a and return the concatenated list */
static G_nodeList cat(G_nodeList a, G_nodeList b) {
  if (a==NULL) return b;
  else return G_NodeList(a->head, cat(a->tail, b));
}

/* create the adjacency list for node n by combining the successor and
 * predecessor lists of node n */
G_nodeList G_adj(G_node n) {return cat(G_succ(n), G_pred(n));}

void *G_nodeInfo(G_node n) {return n->info;}

/* G_node table functions */

G_table G_empty(void)
{
    return TAB_empty();
}

void G_enter(G_table t, G_node node, void *value)
{
    TAB_enter(t, node, value);
}

void *G_look(G_table t, G_node node)
{
    return TAB_look(t, node);
}

/* undirected graphs */

UG_graph UG_Graph(void)
{
    UG_graph g = (UG_graph) checked_malloc(sizeof *g);

    g->cnt     = 0;
    g->nodes   = NULL;
    g->last    = NULL;

    return g;
}

UG_node UG_Node(UG_graph g, void *info)
{
    UG_node n = (UG_node)checked_malloc(sizeof *n);

    UG_nodeList p = UG_NodeList(n, NULL);

    assert(g);
    n->graph = g;
    n->key   = g->cnt++;

    if (!g->last)
        g->nodes = g->last = p;
    else
        g->last  = g->last->tail = p;

    n->adj   = NULL;
    n->info  = info;

    return n;
}

UG_nodeList UG_NodeList(UG_node head, UG_nodeList tail)
{
    UG_nodeList n = (UG_nodeList) checked_malloc(sizeof *n);

    n->head = head;
    n->tail = tail;

    return n;
}

bool UG_inNodeList(UG_node a, UG_nodeList l)
{
    for (UG_nodeList p=l; p!=NULL; p=p->tail)
        if (p->head==a)
            return TRUE;
    return FALSE;
}

UG_nodeList UG_reverseNodes(UG_nodeList l)
{
    UG_nodeList nl = NULL;
    for (; l; l = l->tail)
        nl = UG_NodeList(l->head, nl);
    return nl;
}

void UG_addEdge (UG_node n1, UG_node n2)
{
    assert(n1);
    assert(n2);
    assert(n1->graph == n2->graph);

    if (UG_connected(n1, n2))
        return;

    n1->adj = UG_NodeList(n2, n1->adj);
    n2->adj = UG_NodeList(n1, n2->adj);
}

bool UG_connected (UG_node n1, UG_node n2)
{
  return UG_inNodeList(n1, n2->adj);
}

int UG_degree(UG_node n)
{
    int deg = 0;
    for (UG_nodeList p=n->adj; p!=NULL; p=p->tail)
        deg++;
    return deg;
}

UG_table UG_empty(void)
{
    return TAB_empty();
}

void UG_enter(UG_table t, UG_node node, void *value)
{
    TAB_enter(t, node, value);
}

void *UG_look(UG_table t, UG_node node)
{
  return TAB_look(t, node);
}

