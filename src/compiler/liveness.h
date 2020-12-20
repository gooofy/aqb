#ifndef HAVE_LIVENESS_H
#define HAVE_LIVENESS_H

#include "flowgraph.h"

typedef struct Live_graph_       *Live_graph;
typedef struct LG_node_          *LG_node;
typedef struct LG_nodeList_      *LG_nodeList;

struct LG_node_
{
    Temp_temp       temp;

    LG_nodeList     adj;

    // graph coloring support:

    int             degree;
    AS_instrSet     relatedMoves;
    int             spillCost;
    LG_node         alias;
    Temp_temp       color;
};

struct LG_nodeList_
{
    LG_nodeList     tail;
    LG_node         node;
};

struct Live_graph_
{
    LG_nodeList     nodes;

    TAB_table       temp2LGNode;
	AS_instrSet     moveWorklist;
};

Live_graph  Live_liveness         (FG_graph flow);

void        Live_showGraph        (FILE *out, Live_graph g, Temp_map m);
LG_node     Live_temp2Node        (Live_graph g, Temp_temp t);

LG_nodeList LG_NodeList           (LG_node node, LG_nodeList tail);
bool        LG_nodeListContains   (LG_nodeList nl, LG_node node);
LG_nodeList LG_nodeListRemove     (LG_nodeList nl, LG_node node, bool *bRemoved);
void        LG_nodeListPrint      (LG_nodeList nl);

#if 0
LG_nodeSet  LG_NodeSet            (void);
bool        LG_nodeSetAdd         (LG_nodeSet nl, LG_node node);
bool        LG_nodeSetSub         (LG_nodeSet nl, LG_node node);
bool        LG_nodeSetIsEmpty     (LG_nodeSet nl);
bool        LG_nodeSetContains    (LG_nodeSet nl, LG_node node);
#endif

bool        LG_connected          (LG_node n1, LG_node n2);

int         LG_computeDegree      (LG_node n);
LG_node     LG_getAlias           (LG_node n);

#endif
