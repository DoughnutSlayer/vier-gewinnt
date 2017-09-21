#include <stdlib.h>
#include "gameboard.h"
#include "knot.h"
#include "testHelp.h"

void calculateWinPercentage(struct knot *knot)
{
    if (knot->winPercentage >= 0)
    {
        return;
    }

    if (!knot->successors)
    {
        if (knot->gameboard->isWonBy == 2)
        {
            knot->winPercentage = 100;
        }
        else
        {
            knot->winPercentage = 0;
        }
        printKnot(knot, "Leaf");
        return;
    }

    double result = 0;
    for (int i = 0; i < knot->successorsCount; i++)
    {
        calculateWinPercentage(knot->successors[i]);
        if (knot->gameboard->nextPlayer == 1)
        {
            result += knot->successors[i]->winPercentage;
        }
        else
        {
            result = (result > knot->successors[i]->winPercentage) ? result : knot->successors[i]->winPercentage;
        }
    }
    if (knot->gameboard->nextPlayer == 1)
    {
        result = result / knot->successorsCount;
    }
    printKnot(knot, "Branch");
    knot->winPercentage = result;
}
