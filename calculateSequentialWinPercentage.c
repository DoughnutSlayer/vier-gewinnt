#include <stdlib.h>
#include "gameboard.h"
#include "knot.h"

void calculateWinPercentage(struct knot *knot)
{
    if (knot->winPercentageIsCalculated)
    {
        return;
    }

    if (knot->successors == NULL)
    {
        if (knot->gameboard->isWonBy == 2)
        {
            knot->winPercentage = 100;
        }
        else
        {
            knot->winPercentage = 0;
        }
        knot->winPercentageIsCalculated = 1;
        return;
    }

    int successorCount = sizeof(knot->successors) / sizeof(knot->successors[0]);
    double winPercentageSum = 0;
    for (int i = 0; i < successorCount; i++)
    {
        calculateWinPercentage(knot->successors[i]);
        winPercentageSum += knot->successors[i]->winPercentage;
    }
    knot->winPercentage = winPercentageSum / successorCount;
    knot->winPercentageIsCalculated = 1;
}
