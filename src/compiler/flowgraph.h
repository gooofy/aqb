#ifndef HAVE_FLOWGRAPH_H
#define HAVE_FLOWGRAPH_H

#include "temp.h"
#include "assem.h"

/*
 * flowgraph.h - Function prototypes and data structures to represent control
 *               flow graphs tailored towards liveness analysis
 */

/*
#define FG_DEPTH_FIRST_ORDER
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

    Temp_tempSet  srcInterf;
    Temp_tempSet  dstInterf;

    Temp_tempSet  def;
    Temp_tempSet  use;

    Temp_tempSet  in;
    Temp_tempSet  out;
#ifdef FG_DEPTH_FIRST_ORDER
    bool          mark; // used in depth-first ordering
#endif
};

FG_nodeList FG_NodeList       (FG_node head, FG_nodeList tail);

FG_graph    FG_AssemFlowGraph (AS_instrList il);
void        FG_free           (FG_graph g);
void        FG_show           (FILE *out, FG_graph g);

#endif
