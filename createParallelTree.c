#include <math.h>
#include <mpi.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "createSequentialTree.h"
#include "gameboard.h"
#include "knot.h"
#include "testHelp.h"

extern const int boardWidth;

struct knot **(turns[BOARD_WIDTH * BOARD_HEIGHT]);

int turnSizes[BOARD_WIDTH * BOARD_HEIGHT];
int turnCounter = 0;

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

    if (knot->successorsCount == 0)
    {
        free(knot->successors);
        knot->successors = NULL;
    }
}

void pSetNextKnots()
{
    nextKnots = malloc(sizeof(currentKnots) * currentKnotsCount * (boardWidth) * 4);
    for (int currentKnot = 0; currentKnot < currentKnotsCount; currentKnot++)
    {
        pSetSuccessorsOf(currentKnots[currentKnot]);
    }

    if (nextKnotsCount == 0)
    {
        free(nextKnots);
        nextKnots = NULL;
    }
}

struct knot *getCurrentKnot(struct gameboard *board)
{
    struct knot *toFind = malloc(sizeof(*toFind));
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

void refreshQueues()
{
    turns[turnCounter] = currentKnots;
    turnSizes[turnCounter] = currentKnotsCount;
    turnCounter++;
    currentKnots = nextKnots;
    currentKnotsCount = nextKnotsCount;
    nextKnots = malloc(sizeof(nextKnots[0]) * currentKnotsCount * boardWidth);
    nextKnotsCount = 0;
}

void pInitializeQueues(struct knot *root)
{
    currentKnots = malloc(sizeof(root));
    currentKnots[0] = root;
    currentKnotsCount = 1;
    pSetNextKnots(currentKnots);
}

void buildParallelTree(struct knot *startKnot)
{
    int worldSize, rank;
    int firstPlayer = startKnot->gameboard->nextPlayer;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    if (worldSize == 1)
    {
        buildTree(startKnot);
        return;
    }

    MPI_Datatype MPI_GAMEBOARD, MPI_GAMEBOARD_ARRAY;
    int boardBlocklengths[3] = {sizeof(((struct gameboard*)0)->lanes) / sizeof(int),
        sizeof(((struct gameboard*)0)->isWonBy) / sizeof(int),
        sizeof(((struct gameboard*)0)->nextPlayer) / sizeof(int)};
    MPI_Aint boardDisplacements[3] = {offsetof(struct gameboard, lanes),
        offsetof(struct gameboard, isWonBy),
        offsetof(struct gameboard, nextPlayer)};
    MPI_Datatype boardTypes[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(3, boardBlocklengths, boardDisplacements,
        boardTypes, &MPI_GAMEBOARD);
    MPI_Type_commit(&MPI_GAMEBOARD);

    MPI_Type_contiguous(boardWidth + 1, MPI_GAMEBOARD, &MPI_GAMEBOARD_ARRAY);
    MPI_Type_commit(&MPI_GAMEBOARD_ARRAY);

    if (rank == 0)
    {
        pInitializeQueues(startKnot);
    }

    int treeFinished = 0;
    while (!treeFinished)
    {
        struct gameboard *taskSendBuffer = malloc(sizeof(*taskSendBuffer) * (currentKnotsCount));
        int *sendCnts = malloc(sizeof(*sendCnts) * worldSize);
        int *displacements = malloc(sizeof(*sendCnts) * worldSize);
        struct gameboard *taskRecvBuffer;
        int recvCnt;
        struct gameboard (*resultSendBuffer)[BOARD_WIDTH + 1];
        struct gameboard (*resultRecvBuffer)[BOARD_WIDTH + 1];

        if (rank == 0)
        {
            for (int i = 0; i < currentKnotsCount; i++)
            {
                taskSendBuffer[i] = *(currentKnots[i]->gameboard);
            }

            int knotsPerProcess = currentKnotsCount / worldSize;
            int displacement = 0;
            //TODO Limit zum senden
            for(int i = 0; i < worldSize; i++)
            {
                int sendCount = knotsPerProcess;
                if (i < currentKnotsCount % worldSize)
                {
                    sendCount += 1;
                }
                sendCnts[i] = sendCount;
                displacements[i] = displacement;
                displacement += sendCount;
            }
        }
        MPI_Scatter(sendCnts, 1, MPI_INT, &recvCnt, 1, MPI_INT, 0, MPI_COMM_WORLD);
        taskRecvBuffer = (struct gameboard *) malloc(sizeof(*taskRecvBuffer) * recvCnt);
        MPI_Scatterv(taskSendBuffer, sendCnts, displacements, MPI_GAMEBOARD, taskRecvBuffer, recvCnt, MPI_GAMEBOARD, 0, MPI_COMM_WORLD);
        free(taskSendBuffer);

        resultSendBuffer = malloc(sizeof(resultSendBuffer[0]) * recvCnt);
        for (int i = 0; i < recvCnt; i++)
        {
            struct gameboard *currentBoard = &taskRecvBuffer[i];
            resultSendBuffer[i][0] = *currentBoard;
            for (int j = 0; j < BOARD_WIDTH; j++)
            {
                struct gameboard *createdBoard = put(currentBoard, j);
                resultSendBuffer[i][j + 1] = (createdBoard) ? *createdBoard : zeroBoard;
                free(createdBoard);
            }
        }
        free(taskRecvBuffer);

        if (rank == 0)
        {
            resultRecvBuffer = malloc(sizeof(resultRecvBuffer[0]) * currentKnotsCount);
        }
        MPI_Gatherv(resultSendBuffer, recvCnt, MPI_GAMEBOARD_ARRAY, resultRecvBuffer, sendCnts, displacements, MPI_GAMEBOARD_ARRAY, 0, MPI_COMM_WORLD);
        free(sendCnts);
        free(displacements);
        free(resultSendBuffer);

        if (rank == 0)
        {
            for (int i = 0; i < currentKnotsCount; i++)
            {
                struct knot *predecessor = getCurrentKnot(&(resultRecvBuffer[i][0]));
                predecessor->successors = malloc(sizeof(predecessor->successors) * boardWidth);
                predecessor->successorsCount = 0;
                if (!predecessor)
                {
                    break;
                }
                for (int j = 1; j < (boardWidth + 1); j++)
                {
                    if (resultRecvBuffer[i][j].nextPlayer == 0)
                    {
                        continue;
                    }
                    struct knot *successor = malloc(sizeof(*successor));
                    successor->gameboard = malloc(sizeof(*(successor->gameboard)));
                    *(successor->gameboard) = resultRecvBuffer[i][j];
                    calculateHash(successor);
                    int duplicateIndex = pCheckForDuplicate(successor);
                    if (duplicateIndex >= 0)
                    {
                        free(successor);
                        successor = nextKnots[duplicateIndex];
                    }
                    else if (!successor->gameboard->isWonBy)
                    {
                        nextKnots[nextKnotsCount] = successor;
                        nextKnotsCount += 1;
                    }
                    else
                    {
                        if (successor->gameboard->isWonBy == 2)
                        {
                            successor->winPercentage = 100;
                        }
                        else
                        {
                            successor->winPercentage = 0;
                        }
                    }
                    predecessor->successors[predecessor->successorsCount] = successor;
                    predecessor->successorsCount += 1;
                }
                if (predecessor->successorsCount == 0)
                {
                    free(predecessor->successors);
                    predecessor->successors = NULL;
                }
            }
            free(resultRecvBuffer);
            if (nextKnotsCount == 0)
            {
                free(nextKnots);
                nextKnots = NULL;
            }
            refreshQueues();
        }
        if (rank == 0 && !currentKnots)
        {
            treeFinished = 1;
        }
        MPI_Bcast(&treeFinished, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }

    MPI_Datatype MPI_WINCHANCE_ARRAY;
    MPI_Type_contiguous(boardWidth, MPI_DOUBLE, &MPI_WINCHANCE_ARRAY);
    MPI_Type_commit(&MPI_WINCHANCE_ARRAY);

    double (*taskSendBuffer)[BOARD_WIDTH];
    int *sendCnts = malloc(sizeof(*sendCnts) * worldSize);
    int *displacements = malloc(sizeof(*displacements) * worldSize);
    int recvCnt;
    double (*taskRecvBuffer)[BOARD_WIDTH];
    double *resultSendBuffer;
    double *resultRecvBuffer;
    turnCounter--;
    MPI_Bcast(&turnCounter, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (int i = turnCounter; i >= 0; i--)
    {
        taskSendBuffer = malloc(sizeof(*taskSendBuffer) * turnSizes[i]);
        if (rank == 0)
        {
            for (int j = 0; j < turnSizes[i]; j++)
            {
                int successorsCount = turns[i][j]->successorsCount;
                for (int k = 0; k < successorsCount; k++)
                {
                    taskSendBuffer[j][k] = turns[i][j]->successors[k]->winPercentage;
                }
                if (successorsCount < boardWidth)
                {
                    taskSendBuffer[j][successorsCount] = (double) -1;
                }
            }

            int knotsPerProcess = turnSizes[i] / worldSize;
            int displacement = 0;
            for (int j = 0; j < worldSize; j++)
            {
                sendCnts[j] = (j < (turnSizes[i] % worldSize)) ? knotsPerProcess + 1 : knotsPerProcess;
                displacements[j] = displacement;
                displacement += sendCnts[j];
            }
        }

        MPI_Scatter(sendCnts, 1, MPI_INT, &recvCnt, 1, MPI_INT, 0, MPI_COMM_WORLD);
        taskRecvBuffer = malloc(sizeof(*taskRecvBuffer) * recvCnt);
        MPI_Scatterv(taskSendBuffer, sendCnts, displacements, MPI_WINCHANCE_ARRAY, taskRecvBuffer, recvCnt, MPI_WINCHANCE_ARRAY, 0, MPI_COMM_WORLD);
        free(taskSendBuffer);

        resultSendBuffer = malloc(sizeof(*resultSendBuffer) * recvCnt);
        for (int j = 0; j < recvCnt; j++)
        {
            double result = 0;
            int resultCount = 0;
            for (int k = 0; k < boardWidth; k++)
            {
                if (taskRecvBuffer[j][k] < 0)
                {
                    break;
                }
                if (i % 2 == firstPlayer % 2)
                {
                    result = fmax(result, taskRecvBuffer[j][k]);
                }
                else
                {
                    result += taskRecvBuffer[j][k];
                    resultCount++;
                }
            }
            if (!(i % 2 == firstPlayer % 2))
            {
                result = result / resultCount;
            }
            resultSendBuffer[j] = result;
        }
        free(taskRecvBuffer);

        resultRecvBuffer = malloc(sizeof(*resultRecvBuffer) * turnSizes[i]);
        MPI_Gatherv(resultSendBuffer, recvCnt, MPI_DOUBLE, resultRecvBuffer, sendCnts, displacements, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        free(resultSendBuffer);

        if (rank == 0)
        {
            for (int j = 0; j < turnSizes[i]; j++)
            {
                turns[i][j]->winPercentage = resultRecvBuffer[j];
            }
        }
        free(resultRecvBuffer);
        break;
    }
}
