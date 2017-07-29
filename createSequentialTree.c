#include <stdlib.h>
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth;

//Queue 1 for current knots
struct knot** currentKnots;
//Queue 2 for next knots
struct knot** nextKnots;

int currentCountOfAddedSucessors;

void initializeCurrentKnots(struct knot* startKnot)
{
    currentKnots = malloc(startKnot * sizeof(startKnot));
    currentKnots[0] = startKnot;
}

int checkForDuplicate(struct knot knot)
{
    for (int i = 0; i < sizeof(nextKnots)/sizeof(struct knot*); i++)
    {
        if (!strcmp(nextKnots[i]->gameboardHash, knot.gameboardHash))
        {
            return i;
        }
    }
    return -1;
}

void setSuccessors(struct knot* knot)
{
    knot->successors = malloc(sizeof(struct knot*) * boardWidth);
    for (int lane = 0; lane < boardWidth; lane++)
    {
        struct knot successor = malloc(sizeof(struct knot));
        successor.gameboard = put(knot->gameboard, lane);
        if (successor.gameboard != NULL)
        {
            int duplicateSuccessorIndex = checkForDuplicate(successor);
            if(duplicateSuccessorIndex == -1);
            {
                successor.predecessors[0] = knot;
                //TODO fix this
                knot->successor[lane] = &successor;
                nextKnots[currentCountOfAddedSucessors] = &successor;
                currentCountOfAddedSucessors++;
            }
            else
            {
                struct knot** successorPredecessors = nextKnots[duplicateSuccessorIndex]->predecessors;
                successorPredecessors[sizeof(successorPredecessors)/sizeof(successorPredecessors[0])] = knot;
                knot->successors[lane] = nextKnots[duplicateSuccessorIndex];
            }
        }
        else
        {
            free(successor);
        }
    }
}

void setNextKnots()
{
    currentCountOfAddedSucessors = 0;
    for (int currentKnot = 0; currentKnot < sizeof(currentKnots)/sizeof(currentKnots[0]); currentKnot++)
    {
        setSuccessors(currentKnots[currentKnot]);
    }
}

void buildTree(struct knot* startKnot)
{
    initializeCurrentKnots(startKnot);
    while (sizeof(currentKnots) != 0) //TODO does work?
    {
        setNextKnots();
        free(currentKnots);
        currentKnots = nextKnots;
        nextKnots = malloc(sizeof(currentKnots) * boardWidth);
    }
}
