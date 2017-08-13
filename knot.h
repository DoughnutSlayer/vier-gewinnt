#ifndef KNOT_H
#define KNOT_H

#include "gameboard.h"

struct knot
{
    struct knot **predecessors;
    int predecessorsCount;
    struct knot **successors;
    struct gameboard *gameboard;
    char gameboardHash[(BOARD_WIDTH * BOARD_HEIGHT) + 1];
    double winPercentage; //Between 0-100
    int winPercentageIsCalculated;
};

void calculateHash(struct knot *knot);

#endif
