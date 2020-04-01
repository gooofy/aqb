#ifndef HAVE_LIVENESS_H
#define HAVE_LIVENESS_H

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	G_node src, dst;
	Live_moveList tail;
};

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);

struct Live_graph {
	G_graph graph;
	Live_moveList moves;
	AS_instrList worklistMoves;
	Temp_map moveList;
	Temp_map spillCost;
};
Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

void Live_showGraph(FILE *out, G_nodeList p, Temp_map m);

#endif
