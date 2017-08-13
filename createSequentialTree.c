#include <stdlib.h>
#include <string.h>
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth;

//Queue 1 for current knots
struct knot **currentKnots;
//Queue 2 for next knots
struct knot **nextKnots;

int allSuccessorsCount;

int checkForDuplicate(struct knot *knot)
{
    for (int i = 0; i < sizeof(nextKnots)/sizeof(struct knot *); i++)
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
    knot->successors = malloc(sizeof(struct knot *) * boardWidth);
    for (int lane = 0; lane < boardWidth; lane++)
    {
        struct knot *successor = malloc(sizeof(struct knot));
        successor->gameboard = put(knot->gameboard, lane);
        if (successor->gameboard != NULL)
        {
            int duplicateSuccessorIndex = checkForDuplicate(successor);
            if(duplicateSuccessorIndex == -1)
            {
                successor->gameboardHash = calculateHash(*(successor->gameboard));
                successor->predecessors = malloc(sizeof(knot));
                successor->predecessors[0] = knot;
                knot->successors[knotSuccessorsCount] = successor;
                nextKnots[allSuccessorsCount] = successor;
                allSuccessorsCount++;
            }
            else
            {
                free(successor);
                successor = nextKnots[duplicateSuccessorIndex];
                successor->predecessors = realloc(successor->predecessors, sizeof(successor->predecessors) + sizeof(successor->predecessors[0]));
                successor->predecessors[sizeof(successor->predecessors)/sizeof(successor->predecessors[0])] = knot;
                knot->successors[knotSuccessorsCount] = successor;
            }
            knotSuccessorsCount++;
        }
        else
        {
            free(successor);
        }
    }
    knot->successors = realloc(knot->successors, sizeof(struct knot *) * knotSuccessorsCount);
}

void setNextKnots()
{
    nextKnots = malloc(sizeof(currentKnots) * boardWidth);
    allSuccessorsCount = 0;
    for (int currentKnot = 0; currentKnot < sizeof(currentKnots)/sizeof(currentKnots[0]); currentKnot++)
    {
        setSuccessorsOf(currentKnots[currentKnot]);
    }
    nextKnots = realloc(nextKnots, allSuccessorsCount);
}

void initializeQueues(struct knot *startKnot)
{
    nextKnots = malloc(sizeof(startKnot));
    nextKnots[0] = startKnot;
    currentKnots = malloc(1);
}

void buildTree(struct knot *startKnot)
{
    initializeQueues(startKnot);
    while (nextKnots != NULL)
    {
        free(currentKnots);
        currentKnots = nextKnots;
        setNextKnots();
    }
    //Hier sind die currentKnots die letzte Generation von Knoten
}
