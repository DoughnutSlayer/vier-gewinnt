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

struct gameboard zeroBoard = {0};

int currentKnotsCount;

int nextKnotsCount;

int pCheckForDuplicate(struct knot *knot)
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

void pSetSuccessorsOf(struct knot *knot)
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
            int duplicateSuccessorIndex = pCheckForDuplicate(successor);
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

void pSetNextKnots()
{
    int currentKnotsCount = nextKnotsCount;
    nextKnotsCount = 0;
    nextKnots = malloc(sizeof(currentKnots) * currentKnotsCount * boardWidth);
    for (int currentKnot = 0; currentKnot < currentKnotsCount; currentKnot++)
    {
        pSetSuccessorsOf(currentKnots[currentKnot]);
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

struct knot *getCurrentKnot(struct gameboard *board)
{
    struct knot *result = NULL;
    struct knot *toFind = malloc(sizeof(*toFind);
    toFind->gameboard = board;
    calculateHash(toFind);
    for (int i = 0; i < currentKnotsCount; i++)
    {
        if (!strcmp(currentKnots[i]->gameboardHash, toFind->gameboardHash))
        {
            free(toFind);
            return currentKnots[i];
        }
    }
    return NULL;
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
        pSetNextKnots(currentKnots);
        currentKnots = nextKnots;
        currentKnotsCount = nextKnotsCount;
        //end of init
    }

    while (currentKnots)
    {
        struct gameboard *sendBuffer = malloc(sizeof(*sendBuffer) * currentKnotsCount);
        int *sendCnts;
        int *displacements;
        int *recvBuffer;
        int recvCnt;
        struct gameboard (*gameboardRecvBuffer)[BOARD_WIDTH + 1];
        if (rank == 0)
        {
            for (int i = 0; i < currentKnotsCount; i++)
            {
                sendBuffer[i] = currentKnots[i]->gameboard;
            }

            int knotsPerProcess = currentKnotsCount / worldSize;
            int displacement = 0;
            //TODO Limit zum senden
            for(i = 0; i < worldSize; i++)
            {
                int j = knotsPerProcess;
                if (rank < currentKnotsCount % worldSize)
                {
                    j += 1;
                }
                sendCnts[i] = j;
                displacements[i] = displacement;
                displacement += j;
            }
        }

        MPI_Scatter(sendCnts, 1, MPI_INT, &recvCnt, MPI_INT, 0, MPI_COMM_WORLD);

        recvBuffer = malloc(sizeof(*recvBuffer) * recvCnt);
        MPI_Scatterv(sendBuffer, sendCnts, displacements, MPI_GAMEBOARD,
            recvBuffer, recvCnt, MPI_GAMEBOARD, 0, MPI_COMM_WORLD);

        struct gameboard (*toSendGameboards)[BOARD_WIDTH + 1] = malloc(sizeof(toSendGameboards[0]) * recvCnt);
        for (int i = 0; i < recvCnt; i++)
        {
            struct gameboard *currentBoard = recvBuffer + i;
            toSendGameboards[i][0] = currentBoard;
            for (int j = 0; j < BOARD_WIDTH; j++)
            {
                struct gameboard *createdBoard = put(currentBoard, j);
                toSendGameboards[i][j + 1] = (createdBoard) ? *createdBoard : zeroBoard;
            }
        }

        if (rank == 0)
        {
            gameboardRecvBuffer = malloc(sizeof(gameboardRecvBuffer[0]) * currentKnotsCount);
        }
        MPI_Gatherv(toSendGameboards, sendCnts[rank], MPI_GAMEBOARD_ARRAY, gameboardRecvBuffer, sendCnts, displacements, MPI_GAMEBOARD_ARRAY, 0, MPI_COMM_WORLD);

        if (rank == 0)
        {
            for (int i = 0; i < currentKnotsCount; i++)
            {
                struct knot *predecessor = getCurrentKnot(&(gameboardRecvBuffer[i][0]));
                if (!predecessor)
                {
                    break;
                }
                for (int j = 1; j < (boardWidth + 1); j++)
                {
                    if (gameboardRecvBuffer[i][j].nextPlayer == 0)
                    {
                        break;
                    }
                    struct knot *successor = malloc(sizeof(*successor));
                    successor->gameboard = gameboardRecvBuffer[i][j];
                    calculateHash(successor);
                    int duplicateIndex = pCheckForDuplicate(successor);
                    if (duplicateIndex < 0)
                    {
                        successor->successors = malloc(sizeof(successor->successors) * boardWidth)
                        successor->successorsCount = 0;
                        nextKnots[nextKnotsCount] = successor;
                        nextKnotsCount += 1;
                    }
                    else
                    {
                        free(successor);
                        successor = nextKnots[duplicateIndex];
                    }
                    predecessor->successors[predecessor->successorsCount] = successor;
                    predecessor->successorsCount += 1;
                }
                if (predecessor->successorsCount > 0)
                {
                    realloc(predecessor->successors, sizeof(predecessor->successors[0]) * predecessor->successorsCount)
                }
                else
                {
                    free(predecessor->successors);
                    predecessor->successors = NULL;
                }
            }
            free(gameboardRecvBuffer);
            if (nextKnotsCount > 0)
            {
                realloc(nextKnots, sizeof(nextKnots[0]) * nextKnotsCount)
            }
            else
            {
                free(nextKnots);
                nextKnots = NULL;
            }
            currentKnots = nextKnots;
            currentKnotsCount = nextKnotsCount;
        }

        for (int i = 0; i < sendCnts[rank]; i++)
        {
            for (int j = 0; j < (BOARD_WIDTH + 1); j++)
            {
                free(toSendGameboards[i][j]);
            }
        }
        free(toSendGameboards);
    }
    MPI_Finalize();
}
