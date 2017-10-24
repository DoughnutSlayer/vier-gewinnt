#ifndef KNOT_H
#define KNOT_H

struct knot
{
    int successorIndices[BOARD_WIDTH];
    double winPercentage; //Between 0-100
};
#endif
