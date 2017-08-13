#include <stdlib.h>
#include <string.h>
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth;

//Queue 1 for current knots
struct knot **currentKnots;
//Queue 2 for next knots
struct knot **nextKnots;

int nextKnotsCount;

int checkForDuplicate(struct knot *knot)
{
    for (int i = 0; i < nextKnotsCount; i++)
    {
        if (!strcmp(nextKnots[i]->gameboardHash, knot->gameboardHash))
        {
            return i;
        }
    }
    return -1;
}

void setSuccessorsOf(struct knot *knot)
{
    //The number of successors that "knot" currently has
    int knotSuccessorsCount = 0;
    knot->successors = malloc(sizeof(knot->successors) * boardWidth);
    for (int lane = 0; lane < boardWidth; lane++)
    {
        struct knot *successor = malloc(sizeof(*successor));
        successor->gameboard = put(knot->gameboard, lane);
        if (successor->gameboard != NULL)
        {
            calculateHash(successor);
            int duplicateSuccessorIndex = checkForDuplicate(successor);
            if(duplicateSuccessorIndex == -1)
            {
                successor->predecessors = malloc(sizeof(knot));
                successor->predecessors[0] = knot;
                successor->predecessorsCount = 1;
                knot->successors[knotSuccessorsCount] = successor;
                nextKnots[nextKnotsCount] = successor;
                nextKnotsCount++;
            }
            else
            {
                free(successor);
                successor = nextKnots[duplicateSuccessorIndex];
                successor->predecessorsCount++;
                successor->predecessors = realloc(successor->predecessors, sizeof(knot) * successor->predecessorsCount);
                successor->predecessors[successor->predecessorsCount - 1] = knot;
                knot->successors[knotSuccessorsCount] = successor;
            }
            knotSuccessorsCount++;
        }
        else
        {
            free(successor);
        }
    }
    knot->successors = realloc(knot->successors, sizeof(knot) * knotSuccessorsCount);
}

void setNextKnots()
{
    int currentKnotsCount = nextKnotsCount;
    nextKnotsCount = 0;
    nextKnots = malloc(sizeof(currentKnots) * currentKnotsCount * boardWidth);
    for (int currentKnot = 0; currentKnot < currentKnotsCount; currentKnot++)
    {
        setSuccessorsOf(currentKnots[currentKnot]);
    }
    nextKnots = realloc(nextKnots, sizeof(nextKnots) * nextKnotsCount);
}

void initializeQueues(struct knot *startKnot)
{
    nextKnots = malloc(sizeof(startKnot));
    nextKnots[0] = startKnot;
    nextKnotsCount = 1;
    currentKnots = malloc(1);
}

void buildTree(struct knot *startKnot)
{
    initializeQueues(startKnot);
    do
    {
        free(currentKnots);
        currentKnots = nextKnots;
        setNextKnots();
    }
    while (nextKnotsCount > 0);
    //Hier sind die currentKnots die letzte Generation von Knoten
}
