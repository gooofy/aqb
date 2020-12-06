#ifndef HAVE_FLOWGRAPH_H
#define HAVE_FLOWGRAPH_H

#include "temp.h"
#include "assem.h"
#include "frame.h"

/*
 * flowgraph.h - Function prototypes and data structures to represent control
 *               flow graphs tailored towards liveness analysis
 */

typedef struct FG_graph_    *FG_graph;
typedef struct FG_node_     *FG_node;
typedef struct FG_nodeList_ *FG_nodeList;

struct FG_graph_
{
    int          nodecount;
	FG_nodeList  nodes, last_node;
};

struct FG_nodeList_
{
    FG_node     head;
    FG_nodeList tail;
};

struct FG_node_
{
    FG_graph      graph;
    int           key;
    FG_nodeList   succs;
    FG_nodeList   preds;

    AS_instr      instr;

    // liveness analysis:
    Temp_tempSet  in;
    Temp_tempSet  out;
};

FG_nodeList FG_NodeList(FG_node head, FG_nodeList tail);
FG_graph    FG_AssemFlowGraph(AS_instrList il, F_frame f);

void        FG_show (FILE *out, FG_graph g, Temp_map tm);

#endif
