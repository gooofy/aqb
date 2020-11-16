#ifndef HAVE_LIVENESS_H
#define HAVE_LIVENESS_H

#include "flowgraph.h"

typedef struct Live_graph_ *Live_graph;
struct Live_graph_
{
	UG_graph      graph;
	AS_instrList  worklistMoves;
	Temp_map      moveList;
	Temp_map      spillCost;
};
Temp_temp  Live_gtemp(UG_node n);

Live_graph Live_liveness(FG_graph flow);

void       Live_showGraph(FILE *out, Live_graph g, Temp_map m);

#endif
