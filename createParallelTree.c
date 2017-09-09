#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include "createSequentialTree.h"
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth;

//Queue 1 for current knots
struct knot **currentKnots;
//Queue 2 for next knots
struct knot **nextKnots;

int currentKnotsCount;

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
                successor->predecessors = malloc(sizeof(knot));
                successor->predecessors[0] = knot;
                successor->predecessorsCount = 1;
                successor->winPercentage = -1;
                knot->successors[knot->successorsCount] = successor;
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

void initMpiTypes(MPI_Datatype *boardType, MPI_Datatype *boardArrayType)
{
    int boardBlocklengths[3] = {sizeof(((struct gameboard*)0)->lanes),
        sizeof(((struct gameboard*)0)->isWonBy),
        sizeof(((struct gameboard*)0)->nextPlayer)};
    MPI_Aint boardDisplacements[3] = {offsetof(struct gameboard, lanes),
        offsetof(struct gameboard, isWonBy),
        offsetof(struct gameboard, nextPlayer)};
    MPI_Datatype boardTypes[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(3, boardBlocklengths, boardDisplacements,
        boardTypes, boardType);
    MPI_Type_commit(boardType);

    MPI_Type_contiguous(boardWidth + 1, *boardType, boardArrayType);
    MPI_Type_commit(boardArrayType);
}

void buildParallelTree(int argc, char const *argv[], struct knot *startKnot)
{
    int worldSize, rank;
    MPI_INIT(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    if (worldSize == 1)
    {
        buildTree(knot);
        MPI_Finalize();
        return;
    }

    MPI_Datatype MPI_GAMEBOARD, MPI_GAMEBOARD_ARRAY;
    initMpiTypes(&MPI_GAMEBOARD, &MPI_GAMEBOARD_ARRAY);

    if (rank == 0)
    {
        currentKnots = malloc(sizeof(startKnot));
        currentKnots[0] = startKnot;
        currentKnotsCount = 1;
        setNextKnots(currentKnots);
        currentKnots = nextKnots;
        currentKnotsCount = nextKnotsCount;
        //end of init
    }

    while (nextKnots)
    {
        struct gameboard *sendBuffer = malloc(sizeof(*sendBuffer) * currentKnotsCount);
        int *sendCnts;
        int *ones = malloc(sizeof(*ones) * worldSize);
        int *onesDispls = malloc(sizeof(*onesDispls) * worldSize);
        int *displacements;
        int displacement = 0;
        int *recvBuffer;
        int recvCnt;
        struct gameboard **gameboardRecvBuffer;
        if (rank == 0)
        {
            for (int i = 0; i < currentKnotsCount; i++)
            {
                sendBuffer[i] = currentKnots[i]->gameboard;
            }

            //TODO Limit zum senden
            for(i = 0; i < worldSize; i++)
            {
                int k = currentKnotsCount/worldSize;
                if(rank < currentKnotsCount % worldSize)
                {
                    k++;
                }
                sendCnts[i] = k;
                displacements[i] = displacement;
                displacement += k;
                ones[i] = 1;
                onesDispls[i] = i;
            }
            gameboardRecvBuffer = malloc(sizeof(*gameboardRecvBuffer) * currentKnotsCount * (boardWidth + 1));
        }

        MPI_Scatterv(sendCnts, ones, onesDispls, MPI_INT, &recvCnt, 1, MPI_INT, 0, MPI_COMM_WORLD);

        recvBuffer = malloc(sizeof(*recvBuffer) * recvCnt);
        MPI_Scatterv(sendBuffer, sendCnts, displacements, MPI_GAMEBOARD, recvBuffer, recvCnt, MPI_GAMEBOARD, 0, MPI_COMM_WORLD);

        struct gameboard **toSendGameboards;

        MPI_Gatherv(toSendGameboards, sendCnts[rank], MPI_GAMEBOARD_ARRAY, gameboardRecvBuffer)
        //TODO: Calculate next gameboards

        recvknots += epfangene knots
        nextKnots = recvknots
    }
    MPI_Finalize();
}
