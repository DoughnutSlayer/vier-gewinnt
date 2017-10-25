#include <math.h>
#include <mpi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gameboard.h"
#include "knot.h"
#include "vier-gewinnt.h"

const char gbFileNameA[13] = ".gameboardsA";
const char gbFileNameB[13] = ".gameboardsB";

int rank;
int worldSize;
int firstPlayer;

struct knot *predecessorKnots;
struct knot *successorKnots;

int turnSizes[(BOARD_WIDTH * BOARD_HEIGHT) + 1] = {0};
int turnDisplacements[(BOARD_WIDTH * BOARD_HEIGHT) + 1] = {0};
int turnCounter = 0;

// Queue 1 for current knots
struct gameboard *currentGameboards;
int currentGameboardsCount;
const char (*currentGbFileNamePtr)[13] = &gbFileNameA;
// Queue 2 for next knots
const char (*nextGbFileNamePtr)[13] = &gbFileNameB;

struct gameboard zeroBoard = {0};

void setStartTurn(struct gameboard *startGameboard)
{
    for (int i = 0; i < boardWidth; i++)
    {
        for (int j = 0; j < boardHeight; j++)
        {
            if (startGameboard->lanes[i][j] > 0)
            {
                turnSizes[turnCounter] = 0;
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

void setTurnSize(int turnIndex, int turnSize)
{
    turnSizes[turnIndex] = turnSize;
    setTurnDisplacement(turnIndex);
}

void refreshGameboardFiles()
{
    const char(*buf)[13] = currentGbFileNamePtr;
    currentGbFileNamePtr = nextGbFileNamePtr;
    nextGbFileNamePtr = buf;
    FILE *nextGbFile = fopen(*nextGbFileNamePtr, "wb");
    fclose(nextGbFile);
}

void refreshKnotQueues()
{
    free(predecessorKnots);
    predecessorKnots = successorKnots;
    successorKnots = malloc(sizeof(*successorKnots) * currentGameboardsCount);
}

void initializeQueues(struct gameboard startGameboard)
{
    currentGameboards = malloc(sizeof(startGameboard));
    currentGameboards[0] = startGameboard;
    currentGameboardsCount = 1;

    FILE *currentGbFile = fopen(*currentGbFileNamePtr, "wb");
    fwrite(currentGameboards, sizeof(*currentGameboards), 1, currentGbFile);
    fclose(currentGbFile);
    FILE *nextGbFile = fopen(*nextGbFileNamePtr, "wb");
    fclose(nextGbFile);
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

void prepareBoardSend(int totalSendCount, struct gameboard *boardSendBuffer)
{
    int currentBoardIndex = 0;
    for (int i = 0; i < totalSendCount;)
    {
        if (currentGameboards[i].nextPlayer == 0)
        {
            currentBoardIndex++;
            continue;
        }
        boardSendBuffer[i] = currentGameboards[currentBoardIndex];
        currentBoardIndex++;
        i++;
    }
}

void prepareWinpercentageArraySend(
  int turnIndex, double (*winpercentageArraySendBuffer)[BOARD_WIDTH],
  int *sendCnts, int *displacements)
{
    for (int j = 0; j < turnSizes[turnIndex]; j++)
    {
        struct knot *predecessor = &(predecessorKnots[j]);
        int successorsCount = 0;
        for (int k = 0; k < boardWidth; k++)
        {
            int successorIndex = predecessor->successorIndices[k];
            if (successorIndex < 0)
            {
                continue;
            }
            winpercentageArraySendBuffer[j][successorsCount] =
              successorKnots[successorIndex].winPercentage;
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
    int currentTurnSize = 0;

    for (int i = 0; i < turnSizes[turnCounter - 1]; i++)
    {
        struct knot *predecessor = &(predecessorKnots[i]);
        for (int j = 0; j < boardWidth; j++)
        {
            struct gameboard successorGameboard =
              currentGameboards[(boardWidth * i) + j];
            if (successorGameboard.nextPlayer == 0)
            {
                predecessor->successorIndices[j] = -1;
                continue;
            }

            struct knot *successor = &(successorKnots[currentTurnSize]);
            if (successorGameboard.isWonBy == 2)
            {
                successor->winPercentage = 100;
            }
            else
            {
                successor->winPercentage = 0;
            }
            predecessor->successorIndices[j] = currentTurnSize;
            currentTurnSize++;
        }
    }
    setTurnSize(turnCounter, currentTurnSize);

    FILE *knotsFile = fopen(saveFileName, "ab");
    fwrite(predecessorKnots, sizeof(*predecessorKnots),
           turnSizes[turnCounter - 1], knotsFile);
    fclose(knotsFile);
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

void saveNextGameboards(int totalRecvCount,
                        struct gameboard (*successorArrays)[BOARD_WIDTH])
{
    FILE *nextGbFile = fopen(*nextGbFileNamePtr, "ab");
    for (int i = 0; i < totalRecvCount; i++)
    {
        fwrite(successorArrays[i], sizeof(**successorArrays), boardWidth,
               nextGbFile);
    }
    fclose(nextGbFile);
}

void nextTurn()
{
    refreshGameboardFiles();
    refreshKnotQueues();
    turnCounter++;
}

void saveLastTurn()
{
    for (int i = 0; i < turnSizes[turnCounter]; i++)
    {
        for (int j = 0; j < boardWidth; j++)
        {
            successorKnots[i].successorIndices[j] = -1;
        }
    }
    FILE *saveFile = fopen(saveFileName, "ab");
    fwrite(successorKnots, sizeof(*successorKnots), turnSizes[turnCounter],
           saveFile);
    fclose(saveFile);
}

int calculateTurnSteps(int recvCnt, int totalSendCount)
{
    long unsigned int gbSize = sizeof(struct gameboard);
    struct gameboard gbArray[BOARD_WIDTH];
    long unsigned int gbArrSize = sizeof(gbArray);
    long unsigned int knotSize = sizeof(struct knot);

    int buffer = 0;
    int sufficientMemory = 0;
    int turnSteps = 1;
    long unsigned int neededBytes = 0;

    if (rank == 0)
    {
        neededBytes +=
          gbSize * currentGameboardsCount                // taskSendBuffer
          + gbArrSize * totalSendCount                   // resultRecvBuffer
          - gbSize * currentGameboardsCount              // currentGameboards
          + gbSize * currentGameboardsCount * boardWidth // nextGameboards
          - knotSize * turnSizes[turnCounter - 1]        // predecessorKnots
          + knotSize * currentGameboardsCount;           // successorKnots
    }
    neededBytes += gbSize * recvCnt      // taskRecvBuffer
                   + gbArrSize * recvCnt // resultSendBuffer
                   + gbSize              // createdBoard
                   + 1000;               // buffer

    while (!sufficientMemory)
    {
        void *placeholder = malloc((neededBytes / turnSteps) + 1);
        buffer = (placeholder) ? 1 : 0;

        for (int i = 0; i < worldSize; i++)
        {
            if (rank == i)
            {
                sufficientMemory = buffer;
            }
            MPI_Bcast(&sufficientMemory, 1, MPI_INT, i, MPI_COMM_WORLD);
            if (!sufficientMemory)
            {
                turnSteps++;
                break;
            }
        }
        free(placeholder);
    }
    return turnSteps;
}

void calculateSendCounts(int *sendCnts, int *displacements, int *totalSendCount)
{
    *totalSendCount = 0;
    for (int i = 0; i < currentGameboardsCount; i++)
    {
        if (currentGameboards[i].nextPlayer)
        {
            *totalSendCount += 1;
        }
    }

    int sendPerProcess = *totalSendCount / worldSize;
    for (int i = 0; i < worldSize; i++)
    {
        sendCnts[i] = sendPerProcess;
        if (i < *totalSendCount % worldSize)
        {
            sendCnts[i] += 1;
        }

        if (i == 0)
        {
            displacements[i] = 0;
        }
        else
        {
            displacements[i] = displacements[i - 1] + sendCnts[i - 1];
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
            calculateSendCounts(sendCnts, displacements, &totalSendCount);
        }
        MPI_Scatter(sendCnts, 1, MPI_INT, &recvCnt, 1, MPI_INT, 0,
                    MPI_COMM_WORLD);
        MPI_Scatter(displacements, 1, MPI_INT, &displacement, 1, MPI_INT, 0,
                    MPI_COMM_WORLD);
        MPI_Bcast(&totalSendCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
        int turnSteps = calculateTurnSteps(recvCnt, totalSendCount);

        if (rank == 0)
        {
            taskSendBuffer = malloc(sizeof(*taskSendBuffer) * totalSendCount);
            prepareBoardSend(totalSendCount, taskSendBuffer);
        }

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
                if (!(nextGameboards[i].isWonBy)
                    && nextGameboards[i].nextPlayer)
                {
                    treeFinished = 0;
                    break;
                }
            }
            nextTurn();
            if (treeFinished)
            {
                addCurrentGameboardsTurn();
                saveLastTurn();

                free(nextGameboards);
                nextGameboards = NULL;
                free(predecessorKnots);
                predecessorKnots = NULL;
            }
        }
        MPI_Bcast(&treeFinished, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    free(sendCnts);
    free(displacements);
}

void loadKnotQueues(int turnIndex)
{
    FILE *saveFile = fopen(saveFileName, "rb");

    if (predecessorKnots)
    {
        successorKnots = predecessorKnots;
    }
    else
    {
        successorKnots =
          malloc(sizeof(*successorKnots) * turnSizes[turnIndex + 1]);
        fseek(saveFile,
              sizeof(*successorKnots) * turnDisplacements[turnIndex + 1],
              SEEK_SET);
        fread(successorKnots, sizeof(*successorKnots), turnSizes[turnIndex + 1],
              saveFile);
    }
    predecessorKnots = malloc(sizeof(*predecessorKnots) * turnSizes[turnIndex]);

    fseek(saveFile, sizeof(*predecessorKnots) * turnDisplacements[turnIndex],
          SEEK_SET);
    fread(predecessorKnots, sizeof(*predecessorKnots), turnSizes[turnIndex],
          saveFile);
    fclose(saveFile);
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
            loadKnotQueues(turnIndex);
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
                    predecessorKnots[i].winPercentage = resultRecvBuffer[i];
                }
            }

            FILE *saveFile = fopen(saveFileName, "r+b");
            fseek(saveFile,
                  turnDisplacements[turnIndex] * sizeof(*predecessorKnots),
                  SEEK_SET);
            fwrite(predecessorKnots, sizeof(*predecessorKnots),
                   turnSizes[turnIndex], saveFile);
            fclose(saveFile);
        }
        free(resultRecvBuffer);
    }
    free(sendCnts);
    free(displacements);
}

void makeFirstTurn(struct knot *startKnot)
{
    successorKnots = malloc(sizeof(*startKnot));
    successorKnots[0] = *startKnot;
    setTurnSize(turnCounter, 1);

    struct gameboard(*resultBuffer)[BOARD_WIDTH] =
      malloc(sizeof(*resultBuffer));
    calculateBoardSuccessors(1, currentGameboards, 0, resultBuffer);
    saveNextGameboards(1, resultBuffer);

    free(resultBuffer);
    nextTurn();
}

void buildParallelTree(struct knot *startKnot, struct gameboard *startGameboard)
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
        initializeQueues(*startGameboard);
        makeFirstTurn(startKnot);

        FILE *knotsFile = fopen(saveFileName, "wb");
        fclose(knotsFile);
    }
    MPI_Bcast(&firstPlayer, 1, MPI_INT, 0, MPI_COMM_WORLD);

    calculateTurns(&MPI_GAMEBOARD, &MPI_GAMEBOARD_ARRAY);
    calculateWinpercentages(&MPI_WINCHANCE_ARRAY);
}
