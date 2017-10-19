#include <math.h>
#include <mpi.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "gameboard.h"
#include "knot.h"
#include "vier-gewinnt.h"

int rank;
int worldSize;
int firstPlayer;

struct knot **predecessorKnots;
struct knot **successorKnots;

int turnSizes[(BOARD_WIDTH * BOARD_HEIGHT) + 1] = {0};
int turnDisplacements[(BOARD_WIDTH * BOARD_HEIGHT) + 1] = {0};
int turnCounter = 0;

// Queue 1 for current knots
struct gameboard **currentGameboards;
// Queue 2 for next knots
struct gameboard **nextGameboards;

struct gameboard zeroBoard = {0};

int currentGameboardsCount;

int nextGameboardsCount;

void setStartTurn(struct gameboard *startGameboard)
{
    for (int i = 0; i < boardWidth; i++)
    {
        for (int j = 0; j < boardHeight; j++)
        {
            if (startGameboard->lanes[i][j] > 0)
            {
                turnCounter++;
            }
        }
    }
}

void setTurnDisplacement(int turnIndex)
{
    if (turnIndex == 0)
    {
        turnDisplacements[0] = 0;
    }
    else
    {
        turnDisplacements[turnIndex] =
          turnDisplacements[turnIndex - 1] + turnSizes[turnIndex - 1];
    }
}

void deleteCurrentGameboards()
{
    for (int i = 0; i < currentGameboardsCount; i++)
    {
        free(currentGameboards[i]);
    }
    free(currentGameboards);
}

void refreshGameboardQueues()
{
    deleteCurrentGameboards();
    currentGameboards = nextGameboards;
    currentGameboardsCount = nextGameboardsCount;
    nextGameboards =
      malloc(sizeof(nextGameboards[0]) * currentGameboardsCount * boardWidth);
    nextGameboardsCount = 0;
}

void initializeQueues(struct gameboard *startGameboard)
{
    currentGameboards = malloc(sizeof(startGameboard));
    currentGameboards[0] = startGameboard;
    currentGameboardsCount = 1;
    nextGameboards =
      malloc(sizeof(*nextGameboards) * currentGameboardsCount * boardWidth);
    nextGameboardsCount = 0;
}

void defineMPIDatatypes(MPI_Datatype *boardType, MPI_Datatype *boardArrayType,
                        MPI_Datatype *winChanceArrayType)
{
    int boardBlocklengths[4] = {
      sizeof(((struct gameboard *) 0)->predecessorIndex) / sizeof(int),
      sizeof(((struct gameboard *) 0)->lanes) / sizeof(int),
      sizeof(((struct gameboard *) 0)->isWonBy) / sizeof(int),
      sizeof(((struct gameboard *) 0)->nextPlayer) / sizeof(int)};
    MPI_Aint boardDisplacements[4] = {
      offsetof(struct gameboard, predecessorIndex),
      offsetof(struct gameboard, lanes), offsetof(struct gameboard, isWonBy),
      offsetof(struct gameboard, nextPlayer)};
    MPI_Datatype boardTypes[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(4, boardBlocklengths, boardDisplacements, boardTypes,
                           boardType);
    MPI_Type_commit(boardType);

    MPI_Type_contiguous(boardWidth, *boardType, boardArrayType);
    MPI_Type_commit(boardArrayType);

    MPI_Type_contiguous(boardWidth, MPI_DOUBLE, winChanceArrayType);
    MPI_Type_commit(winChanceArrayType);
}

void prepareBoardSend(int *totalSendCount, struct gameboard *boardSendBuffer,
                      int *sendCnts, int *displacements)
{
    *totalSendCount = 0;
    for (int i = 0; i < currentGameboardsCount; i++)
    {
        if (currentGameboards[i]->nextPlayer == 0)
        {
            continue;
        }
        boardSendBuffer[*totalSendCount] = *(currentGameboards[i]);
        *totalSendCount = *totalSendCount + 1;
    }

    int gameboardsPerProcess = *totalSendCount / worldSize;
    int displacement = 0;
    // TODO Limit zum senden
    for (int i = 0; i < worldSize; i++)
    {
        int sendCount = gameboardsPerProcess;
        if (i < *totalSendCount % worldSize)
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
        struct knot *predecessor = turns[turnIndex][j];
        int successorsCount = 0;
        for (int k = 0; k < boardWidth; k++)
        {
            int successorIndex = predecessor->successorIndices[k];
            if (successorIndex < 0)
            {
                continue;
            }
            winpercentageArraySendBuffer[j][successorsCount] =
              turns[turnIndex + 1][successorIndex]->winPercentage;
            successorsCount++;
        }
        if (successorsCount < boardWidth)
        {
            winpercentageArraySendBuffer[j][successorsCount] = -1;
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
  int boardCount, struct gameboard *boards, int firstPredecessorIndex,
  struct gameboard (*boardSuccessorArrays)[BOARD_WIDTH])
{
    for (int i = 0; i < boardCount; i++)
    {
        struct gameboard *currentBoard = &boards[i];
        for (int j = 0; j < boardWidth; j++)
        {
            struct gameboard *createdBoard = put(currentBoard, j);
            if (createdBoard)
            {
                createdBoard->predecessorIndex = firstPredecessorIndex + i;
                boardSuccessorArrays[i][j] = *createdBoard;
            }
            else
            {
                boardSuccessorArrays[i][j] = zeroBoard;
            }
            free(createdBoard);
        }
    }
}

void addCurrentGameboardsTurn()
{
    turns[turnCounter] = malloc(sizeof(*turns) * currentGameboardsCount);
    turnSizes[turnCounter] = 0;

    for (int i = 0; i < turnSizes[turnCounter - 1]; i++)
    {
        struct knot *predecessor = turns[turnCounter - 1][i];
        for (int j = 0; j < boardWidth; j++)
        {
            struct gameboard *successorGameboard =
              currentGameboards[(boardWidth * i) + j];
            if (successorGameboard->nextPlayer == 0)
            {
                predecessor->successorIndices[j] = -1;
                continue;
            }

            struct knot *successor = malloc(sizeof(*successor));
            if (successorGameboard->isWonBy == 2)
            {
                successor->winPercentage = 100;
            }
            else
            {
                successor->winPercentage = 0;
            }
            predecessor->successorIndices[j] = turnSizes[turnCounter];
            turns[turnCounter][turnSizes[turnCounter]] = successor;
            turnSizes[turnCounter]++;
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
        if (winpercentageArrays[j][0] < 0)
        {
            predecessorWinpercentages[j] = -1;
            continue;
        }
        for (int k = 0; k < boardWidth; k++)
        {
            if (winpercentageArrays[j][k] < 0)
            {
                break;
            }
            if (turnIndex % 2 == firstPlayer % 2)
            {
                result += winpercentageArrays[j][k];
                resultCount++;
            }
            else
            {
                result = fmax(result, winpercentageArrays[j][k]);
            }
        }
        if (turnIndex % 2 == firstPlayer % 2)
        {
            result = result / resultCount;
        }
        predecessorWinpercentages[j] = result;
    }
}

void fillNextGameboards(int totalRecvCount,
                        struct gameboard (*successorArrays)[BOARD_WIDTH])
{
    for (int i = 0; i < totalRecvCount; i++)
    {
        for (int j = 0; j < (boardWidth); j++)
        {
            struct gameboard *successor = malloc(sizeof(*successor));
            *successor = successorArrays[i][j];
            nextGameboards[nextGameboardsCount] = successor;
            nextGameboardsCount++;
        }
    }
}

void calculateTurns(MPI_Datatype *boardType, MPI_Datatype *boardArrayType)
{
    int treeFinished = 0;
    struct gameboard *taskSendBuffer;
    int totalSendCount = 0;
    int *sendCnts = malloc(sizeof(*sendCnts) * worldSize);
    int *displacements = malloc(sizeof(*sendCnts) * worldSize);
    int displacement = 0;
    struct gameboard *taskRecvBuffer;
    int recvCnt;
    struct gameboard(*resultSendBuffer)[BOARD_WIDTH];
    struct gameboard(*resultRecvBuffer)[BOARD_WIDTH];
    while (!treeFinished)
    {
        if (rank == 0)
        {
            taskSendBuffer =
              malloc(sizeof(*taskSendBuffer) * (currentGameboardsCount));
            prepareBoardSend(&totalSendCount, taskSendBuffer, sendCnts,
                             displacements);
        }

        MPI_Scatter(displacements, 1, MPI_INT, &displacement, 1, MPI_INT, 0,
                    MPI_COMM_WORLD);
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
        calculateBoardSuccessors(recvCnt, taskRecvBuffer, displacement,
                                 resultSendBuffer);
        if (rank == 0)
        {
            addCurrentGameboardsTurn();
        }
        free(taskRecvBuffer);

        if (rank == 0)
        {
            resultRecvBuffer =
              malloc(sizeof(resultRecvBuffer[0]) * totalSendCount);
        }
        MPI_Gatherv(resultSendBuffer, recvCnt, *boardArrayType,
                    resultRecvBuffer, sendCnts, displacements, *boardArrayType,
                    0, MPI_COMM_WORLD);
        free(resultSendBuffer);

        if (rank == 0)
        {
            fillNextGameboards(totalSendCount, resultRecvBuffer);
            free(resultRecvBuffer);
            treeFinished = 1;
            for (int i = 0; i < nextGameboardsCount; i++)
            {
                if (!(nextGameboards[i]->isWonBy)
                    && nextGameboards[i]->nextPlayer)
                {
                    treeFinished = 0;
                    break;
                }
            }
            turnCounter++;
            refreshGameboardQueues();
            if (treeFinished)
            {
                addCurrentGameboardsTurn();
                free(nextGameboards);
                nextGameboards = NULL;
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
    turnCounter = turnCounter - 1;
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
            for (int i = 0; i < turnSizes[turnIndex]; i++)
            {
                if (resultRecvBuffer[i] >= 0)
                {
                    turns[turnIndex][i]->winPercentage = resultRecvBuffer[i];
                }
            }
        }
        free(resultRecvBuffer);
    }
    free(sendCnts);
    free(displacements);
}

void makeFirstTurn(struct knot *startKnot)
{
    successorKnots = malloc(sizeof(&startKnot));
    successorKnots[0] = startKnot;
    turnSizes[turnCounter] = 1;
    setTurnDisplacement(turnCounter - 1);

    struct gameboard(*resultBuffer)[BOARD_WIDTH] =
      malloc(sizeof(*resultBuffer));
    calculateBoardSuccessors(1, currentGameboards[0], 0, resultBuffer);
    fillNextGameboards(1, resultBuffer);
    free(resultBuffer);
    refreshGameboardQueues();
    turnCounter++;
}

void buildParallelTree(
  struct knot *startKnot, struct gameboard *startGameboard,
  struct knot **((*turnsPointer)[BOARD_WIDTH * BOARD_HEIGHT]))
{
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    MPI_Datatype MPI_GAMEBOARD, MPI_GAMEBOARD_ARRAY, MPI_WINCHANCE_ARRAY;
    defineMPIDatatypes(&MPI_GAMEBOARD, &MPI_GAMEBOARD_ARRAY,
                       &MPI_WINCHANCE_ARRAY);

    if (rank == 0)
    {
        setStartTurn(startGameboard);
        firstPlayer = (startGameboard->nextPlayer - turnCounter % 2);
        initializeQueues(startGameboard);
        makeFirstTurn(startKnot);

        FILE *knotsFile = fopen(saveFileName, "wb");
        fclose(knotsFile);
    }
    MPI_Bcast(&firstPlayer, 1, MPI_INT, 0, MPI_COMM_WORLD);

    calculateTurns(&MPI_GAMEBOARD, &MPI_GAMEBOARD_ARRAY);
    calculateWinpercentages(&MPI_WINCHANCE_ARRAY);

    memcpy((void *) turnsPointer, (void *) &turns, sizeof(turns));
}
