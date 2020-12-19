#ifndef HAVE_LIVENESS_H
#define HAVE_LIVENESS_H

#include "flowgraph.h"

typedef struct Live_graph_       *Live_graph;
typedef struct LG_node_          *LG_node;
typedef struct LG_nodeSetNode_  *LG_nodeSetNode;
typedef struct LG_nodeSet_      *LG_nodeSet;

struct LG_node_
{
    Temp_temp       temp;

    LG_nodeSet      adj;

    // graph coloring support:

    int             degree;
    AS_instrSet     relatedMoves;
    int             spillCost;
    LG_node         alias;
    Temp_temp       color;
};

struct LG_nodeSetNode_
{
    LG_nodeSetNode  next, prev;
    LG_node         node;
};

struct LG_nodeSet_
{
    LG_nodeSetNode  first, last;
};

struct Live_graph_
{
    LG_nodeSet      nodes;

    TAB_table       temp2LGNode;
	AS_instrSet     moveWorklist;
};

Live_graph  Live_liveness         (FG_graph flow);

void        Live_showGraph        (FILE *out, Live_graph g, Temp_map m);
LG_node     Live_temp2Node        (Live_graph g, Temp_temp t);

LG_nodeSet  LG_NodeSet            (void);
bool        LG_nodeSetAdd         (LG_nodeSet nl, LG_node node);
bool        LG_nodeSetSub         (LG_nodeSet nl, LG_node node);
bool        LG_nodeSetIsEmpty     (LG_nodeSet nl);
bool        LG_nodeSetContains    (LG_nodeSet nl, LG_node node);
void        LG_nodeSetPrint       (LG_nodeSet nl);

bool        LG_connected          (LG_node n1, LG_node n2);

int         LG_computeDegree      (LG_node n);
LG_node     LG_getAlias           (LG_node n);

#endif
