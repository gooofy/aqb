#ifndef HAVE_COLOR_H
#define HAVE_COLOR_H

#include "liveness.h"

/*
 * color.h - Data structures and function prototypes for coloring algorithm
 *           to determine register allocation.
 */

struct COL_result
{
    Temp_map      coloring;
    Temp_tempList colored;
    Temp_tempSet  spills;
    AS_instrSet   coalescedMoves;
    Temp_tempSet  coalescedNodes;
    G_table       alias;
};

struct COL_result COL_color(Live_graph lg, Temp_map initial, Temp_tempList regs);

#endif

