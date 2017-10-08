#include <math.h>
#include <mpi.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "createSequentialTree.h"
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth;

int rank;
int worldSize;
int firstPlayer;

struct knot **(turns[BOARD_WIDTH * BOARD_HEIGHT]);

int turnSizes[BOARD_WIDTH * BOARD_HEIGHT];
int turnCounter = 0;

// Queue 1 for current knots
struct knot **currentKnots;
// Queue 2 for next knots
struct knot **nextKnots;

struct gameboard zeroBoard = {0};

int currentKnotsCount;

int nextKnotsCount;

int getCurrentTurnDuplicateIndex(struct knot *knot)
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

void initializeSuccessors(struct knot *knot)
{
    knot->successorsCount = 0;
    knot->successors = malloc(sizeof(knot->successors) * boardWidth);
    for (int lane = 0; lane < boardWidth; lane++)
    {
        struct knot *successor = malloc(sizeof(*successor));
        successor->gameboard = put(knot->gameboard, lane);
        if (successor->gameboard == NULL)
        {
            free(successor);
            continue;
        }

        calculateHash(successor);
        int duplicateSuccessorIndex = getCurrentTurnDuplicateIndex(successor);
        if (duplicateSuccessorIndex == -1)
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

    if (knot->successorsCount == 0)
    {
        free(knot->successors);
        knot->successors = NULL;
    }
}

void initializeNextKnots()
{
    nextKnots = malloc(sizeof(currentKnots) * currentKnotsCount * boardWidth);
    for (int currentKnot = 0; currentKnot < currentKnotsCount; currentKnot++)
    {
        initializeSuccessors(currentKnots[currentKnot]);
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

void setStartTurn(struct knot *startKnot)
{
    int index = 0;
    while (startKnot->gameboardHash[index] != '\0')
    {
        if (startKnot->gameboardHash[index] != '0')
        {
            turnCounter++;
        }
        index++;
    }
}

void refreshQueues()
{
    currentKnots =
      realloc(currentKnots, sizeof(*currentKnots) * currentKnotsCount);
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
    initializeNextKnots(currentKnots);
}

void defineMPIDatatypes(MPI_Datatype *boardType, MPI_Datatype *boardArrayType,
                        MPI_Datatype *winChanceArrayType)
{
    int boardBlocklengths[3] = {
      sizeof(((struct gameboard *) 0)->lanes) / sizeof(int),
      sizeof(((struct gameboard *) 0)->isWonBy) / sizeof(int),
      sizeof(((struct gameboard *) 0)->nextPlayer) / sizeof(int)};
    MPI_Aint boardDisplacements[3] = {offsetof(struct gameboard, lanes),
                                      offsetof(struct gameboard, isWonBy),
                                      offsetof(struct gameboard, nextPlayer)};
    MPI_Datatype boardTypes[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(3, boardBlocklengths, boardDisplacements, boardTypes,
                           boardType);
    MPI_Type_commit(boardType);

    MPI_Type_contiguous(boardWidth, *boardType, boardArrayType);
    MPI_Type_commit(boardArrayType);

    MPI_Type_contiguous(boardWidth, MPI_DOUBLE, winChanceArrayType);
    MPI_Type_commit(winChanceArrayType);
}

void prepareBoardSend(struct gameboard *boardSendBuffer, int *sendCnts,
                      int *displacements)
{
    for (int i = 0; i < currentKnotsCount; i++)
    {
        boardSendBuffer[i] = *(currentKnots[i]->gameboard);
    }

    int knotsPerProcess = currentKnotsCount / worldSize;
    int displacement = 0;
    // TODO Limit zum senden
    for (int i = 0; i < worldSize; i++)
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

void prepareWinpercentageArraySend(
  int turnIndex, double (*winpercentageArraySendBuffer)[BOARD_WIDTH],
  int *sendCnts, int *displacements)
{
    for (int j = 0; j < turnSizes[turnIndex]; j++)
    {
        int successorsCount = turns[turnIndex][j]->successorsCount;
        for (int k = 0; k < successorsCount; k++)
        {
            winpercentageArraySendBuffer[j][k] =
              turns[turnIndex][j]->successors[k]->winPercentage;
        }
        if (successorsCount < boardWidth)
        {
            winpercentageArraySendBuffer[j][successorsCount] = (double) -1;
        }
    }

    int knotsPerProcess = turnSizes[turnIndex] / worldSize;
    int displacement = 0;
    for (int j = 0; j < worldSize; j++)
    {
        sendCnts[j] = (j < (turnSizes[turnIndex] % worldSize))
                        ? knotsPerProcess + 1
                        : knotsPerProcess;
        displacements[j] = displacement;
        displacement += sendCnts[j];
    }
}

void calculateBoardSuccessors(
  int boardCount, struct gameboard *boards,
  struct gameboard (*boardSuccessorArrays)[BOARD_WIDTH])
{
    for (int i = 0; i < boardCount; i++)
    {
        struct gameboard *currentBoard = &boards[i];
        for (int j = 0; j < boardWidth; j++)
        {
            struct gameboard *createdBoard = put(currentBoard, j);
            boardSuccessorArrays[i][j] =
              (createdBoard) ? *createdBoard : zeroBoard;
            free(createdBoard);
        }
    }
}

void calculatePredecessorWinpercentages(
  int turnIndex, int winpercentageArrayCount,
  double (*winpercentageArrays)[BOARD_WIDTH], double *predecessorWinpercentages)
{
    for (int j = 0; j < winpercentageArrayCount; j++)
    {
        double result = 0;
        int resultCount = 0;
        for (int k = 0; k < boardWidth; k++)
        {
            if (winpercentageArrays[j][k] < 0)
            {
                break;
            }
            if (turnIndex % 2 == firstPlayer % 2)
            {
                result = fmax(result, winpercentageArrays[j][k]);
            }
            else
            {
                result += winpercentageArrays[j][k];
                resultCount++;
            }
        }
        if (!(turnIndex % 2 == firstPlayer % 2))
        {
            result = result / resultCount;
        }
        predecessorWinpercentages[j] = result;
    }
}

void fillNextKnots(struct gameboard (*successorArrays)[BOARD_WIDTH])
{
    for (int i = 0; i < currentKnotsCount; i++)
    {
        struct knot *predecessor = currentKnots[i];
        predecessor->successors =
          malloc(sizeof(predecessor->successors) * boardWidth);
        predecessor->successorsCount = 0;
        if (!predecessor)
        {
            break;
        }
        for (int j = 0; j < (boardWidth); j++)
        {
            if (successorArrays[i][j].nextPlayer == 0)
            {
                continue;
            }
            struct knot *successor = malloc(sizeof(*successor));
            successor->gameboard = malloc(sizeof(*(successor->gameboard)));
            *(successor->gameboard) = successorArrays[i][j];
            calculateHash(successor);
            int duplicateIndex = getCurrentTurnDuplicateIndex(successor);
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
}

void calculateTurns(MPI_Datatype *boardType, MPI_Datatype *boardArrayType)
{
    int treeFinished = 0;
    struct gameboard *taskSendBuffer;
    int *sendCnts = malloc(sizeof(*sendCnts) * worldSize);
    int *displacements = malloc(sizeof(*sendCnts) * worldSize);
    struct gameboard *taskRecvBuffer;
    int recvCnt;
    struct gameboard(*resultSendBuffer)[BOARD_WIDTH];
    struct gameboard(*resultRecvBuffer)[BOARD_WIDTH];
    while (!treeFinished)
    {
        if (rank == 0)
        {
            taskSendBuffer =
              malloc(sizeof(*taskSendBuffer) * (currentKnotsCount));
            prepareBoardSend(taskSendBuffer, sendCnts, displacements);
        }

        MPI_Scatter(sendCnts, 1, MPI_INT, &recvCnt, 1, MPI_INT, 0,
                    MPI_COMM_WORLD);
        taskRecvBuffer =
          (struct gameboard *) malloc(sizeof(*taskRecvBuffer) * recvCnt);
        MPI_Scatterv(taskSendBuffer, sendCnts, displacements, *boardType,
                     taskRecvBuffer, recvCnt, *boardType, 0, MPI_COMM_WORLD);
        if (rank == 0)
        {
            free(taskSendBuffer);
        }

        resultSendBuffer = malloc(sizeof(resultSendBuffer[0]) * recvCnt);
        calculateBoardSuccessors(recvCnt, taskRecvBuffer, resultSendBuffer);
        free(taskRecvBuffer);

        if (rank == 0)
        {
            resultRecvBuffer =
              malloc(sizeof(resultRecvBuffer[0]) * currentKnotsCount);
        }
        MPI_Gatherv(resultSendBuffer, recvCnt, *boardArrayType,
                    resultRecvBuffer, sendCnts, displacements, *boardArrayType,
                    0, MPI_COMM_WORLD);
        free(resultSendBuffer);

        if (rank == 0)
        {
            fillNextKnots(resultRecvBuffer);
            free(resultRecvBuffer);
            if (nextKnotsCount == 0)
            {
                free(nextKnots);
                nextKnots = NULL;
            }
            refreshQueues();
            if (!currentKnots)
            {
                treeFinished = 1;
            }
        }
        MPI_Bcast(&treeFinished, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    free(sendCnts);
    free(displacements);
}

void calculateWinpercentages(MPI_Datatype *winpercentageArrayType)
{
    int recvCnt;
    int *sendCnts = malloc(sizeof(*sendCnts) * worldSize);
    int *displacements = malloc(sizeof(*displacements) * worldSize);
    double(*taskSendBuffer)[BOARD_WIDTH];
    double(*taskRecvBuffer)[BOARD_WIDTH];
    double *resultSendBuffer;
    double *resultRecvBuffer;
    turnCounter--;
    MPI_Bcast(&turnCounter, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (int turnIndex = turnCounter; turnIndex >= 0; turnIndex--)
    {
        taskSendBuffer = malloc(sizeof(*taskSendBuffer) * turnSizes[turnIndex]);
        if (rank == 0)
        {
            prepareWinpercentageArraySend(turnIndex, taskSendBuffer, sendCnts,
                                          displacements);
        }

        MPI_Scatter(sendCnts, 1, MPI_INT, &recvCnt, 1, MPI_INT, 0,
                    MPI_COMM_WORLD);
        taskRecvBuffer = malloc(sizeof(*taskRecvBuffer) * recvCnt);
        MPI_Scatterv(taskSendBuffer, sendCnts, displacements,
                     *winpercentageArrayType, taskRecvBuffer, recvCnt,
                     *winpercentageArrayType, 0, MPI_COMM_WORLD);
        free(taskSendBuffer);

        resultSendBuffer = malloc(sizeof(*resultSendBuffer) * recvCnt);
        calculatePredecessorWinpercentages(turnIndex, recvCnt, taskRecvBuffer,
                                           resultSendBuffer);
        free(taskRecvBuffer);

        resultRecvBuffer =
          malloc(sizeof(*resultRecvBuffer) * turnSizes[turnIndex]);
        MPI_Gatherv(resultSendBuffer, recvCnt, MPI_DOUBLE, resultRecvBuffer,
                    sendCnts, displacements, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        free(resultSendBuffer);

        if (rank == 0)
        {
            for (int j = 0; j < turnSizes[turnIndex]; j++)
            {
                turns[turnIndex][j]->winPercentage = resultRecvBuffer[j];
            }
        }
        free(resultRecvBuffer);
    }
    free(sendCnts);
    free(displacements);
}

void buildParallelTree(struct knot *startKnot)
{
    if (worldSize == 1)
    {
        buildTree(startKnot);
        return;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    MPI_Datatype MPI_GAMEBOARD, MPI_GAMEBOARD_ARRAY, MPI_WINCHANCE_ARRAY;
    defineMPIDatatypes(&MPI_GAMEBOARD, &MPI_GAMEBOARD_ARRAY,
                       &MPI_WINCHANCE_ARRAY);

    if (rank == 0)
    {
        setStartTurn(startKnot);
        firstPlayer = (startKnot->gameboard->nextPlayer - turnCounter % 2);
        pInitializeQueues(startKnot);
    }
    MPI_Bcast(&firstPlayer, 1, MPI_INT, 0, MPI_COMM_WORLD);

    calculateTurns(&MPI_GAMEBOARD, &MPI_GAMEBOARD_ARRAY);
    calculateWinpercentages(&MPI_WINCHANCE_ARRAY);
}
