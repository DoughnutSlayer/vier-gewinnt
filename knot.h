#ifndef KNOT_H
#define KNOT_H

#include "gameboard.h"

struct knot
{
    struct knot **successors;
    int successorsCount;
    struct gameboard *gameboard;
    double winPercentage; //Between 0-100
};
#endif
