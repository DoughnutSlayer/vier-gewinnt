#include <stdlib.h>
#include <string.h>
#include "calculateSequentialWinPercentage.h"
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
    knot->successorsCount = 0;
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
                successor->winPercentage = -1;
                knot->successors[knot->successorsCount] = successor;
                nextKnots[nextKnotsCount] = successor;
                nextKnotsCount++;
            }
            else
            {
                free(successor);
                successor = nextKnots[duplicateSuccessorIndex];
                knot->successors[knot->successorsCount] = successor;
            }
            knot->successorsCount++;
        }
        else
        {
            free(successor);
        }
    }

    if (knot->successorsCount > 0)
    {
        knot->successors = realloc(knot->successors, sizeof(knot) * knot->successorsCount);
    }
    else
    {
        free(knot->successors);
        knot->successors = NULL;
    }
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

    if (nextKnotsCount > 0)
    {
        nextKnots = realloc(nextKnots, sizeof(nextKnots) * nextKnotsCount);
    }
    else
    {
        free(nextKnots);
        nextKnots = NULL;
    }
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
    while (nextKnots)
    {
        free(currentKnots);
        currentKnots = nextKnots;
        setNextKnots();
    }
    //Hier sind die currentKnots die letzte Generation von Knoten
    calculateWinPercentage(startKnot);
}
