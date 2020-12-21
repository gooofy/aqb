#ifndef HAVE_COLOR_H
#define HAVE_COLOR_H

#include "liveness.h"

/*
 * color.h - Data structures and function prototypes for coloring algorithm
 *           to determine register allocation.
 */

struct COL_result
{
    TAB_table     coloring;
    Temp_tempSet  spills;
    AS_instrSet   coalescedMoves;
};

struct COL_result COL_color(Live_graph lg);

#endif

