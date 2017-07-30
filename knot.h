#ifndef KNOT_H
#define KNOT_H

#include "gameboard.h"

struct knot
{
    struct knot **predecessors;
    struct knot **successors;
    struct gameboard *gameboard;
    char gameboardHash[BOARD_WIDTH * BOARD_HEIGHT];
    double winPercentage; //Between 0-100
};

char *calculateHash(struct gameboard *gameboard);

#endif
