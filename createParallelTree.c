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

//Queue 1 for current knots
struct knot **currentKnots;
//Queue 2 for next knots
struct knot **nextKnots;

struct gameboard zeroBoard = {0};

int currentKnotsCount;

int nextKnotsCount;

int pCheckForDuplicate(struct knot *knot)
{
    /*for (int i = 0; i < nextKnotsCount; i++)
    {
        if (!strcmp(nextKnots[i]->gameboardHash, knot->gameboardHash))
        {
            return i;
        }
    }*/
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

/*void initMpiTypes(MPI_Datatype *boardType, MPI_Datatype *boardArrayType)
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
}*/

void buildParallelTree(int argc, char *argv[], struct knot *startKnot)
{
    MPI_Status status;
    MPI_Init(&argc, &argv);
    int worldSize, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    if (worldSize == 1)
    {
        buildTree(startKnot);
        MPI_Finalize();
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
        currentKnots = malloc(sizeof(startKnot));
        currentKnots[0] = startKnot;
        currentKnotsCount = 1;
        pSetNextKnots(currentKnots);
        currentKnots = nextKnots;
        currentKnotsCount = nextKnotsCount;
        nextKnots = malloc(sizeof(nextKnots[0]) * currentKnotsCount * boardWidth);
        nextKnotsCount = 0;
        //end of init
    }

    int treeFinished = 0;
    while (!treeFinished)
    {
        printf("Neue Runde, Neues Glück!\n");
        struct gameboard *sendBuffer = malloc(sizeof(*sendBuffer) * (currentKnotsCount));
        int *sendCnts = malloc(sizeof(*sendCnts) * worldSize);
        int *displacements = malloc(sizeof(*sendCnts) * worldSize);
        struct gameboard *recvBuffer;
        int recvCnt;
        struct gameboard (*gameboardRecvBuffer)[BOARD_WIDTH + 1];

        if (rank == 0)
        {
            for (int i = 0; i < currentKnotsCount; i++)
            {
                sendBuffer[i] = *(currentKnots[i]->gameboard);
            }

            int knotsPerProcess = currentKnotsCount / worldSize;
            int displacement = 0;
            //TODO Limit zum senden
            for(int i = 0; i < worldSize; i++)
            {
                int sendCount = knotsPerProcess;
                if (rank < currentKnotsCount % worldSize)
                {
                    sendCount += 1;
                }
                sendCnts[i] = sendCount;
                displacements[i] = displacement;
                displacement += sendCount;
            }
            //free(sendBuffer);
        }
        MPI_Scatter(sendCnts, 1, MPI_INT, &recvCnt, 1, MPI_INT, 0, MPI_COMM_WORLD);
        recvBuffer = (struct gameboard *) malloc(sizeof(*recvBuffer) * recvCnt);
        MPI_Scatterv(sendBuffer, sendCnts, displacements, MPI_GAMEBOARD, recvBuffer, recvCnt, MPI_GAMEBOARD, 0, MPI_COMM_WORLD);

        if (rank > 0)
        {
            for (int i = 0; i < recvCnt; i++)
            {
                struct gameboard boardArray[BOARD_WIDTH + 1];
                struct gameboard *currentBoard = &recvBuffer[0];
                boardArray[0] = *currentBoard;
                for (int j = 0; j < BOARD_WIDTH; j++)
                {
                    struct gameboard *createdBoard = put(currentBoard, j);
                    boardArray[j + 1] = (createdBoard) ? *createdBoard : zeroBoard;
                    free(createdBoard);
                }
                MPI_Send(&boardArray, 1, MPI_GAMEBOARD_ARRAY, 0, 2, MPI_COMM_WORLD);
            }
            free(recvBuffer);
        }
        else
        {
            gameboardRecvBuffer = malloc(sizeof(gameboardRecvBuffer[0]) * currentKnotsCount);
            for(int i = 0; i < recvCnt; i++)
            {
                gameboardRecvBuffer[i][0] = *(currentKnots[i]->gameboard);
                for(int j = 0; j < BOARD_WIDTH; j++)
                {
                struct gameboard *createdBoard = put(currentKnots[i]->gameboard, j);
                gameboardRecvBuffer[i][j + 1] = (createdBoard) ? *createdBoard : zeroBoard;
                //free(createdBoard);
                }
            }

            for (int i = 1; i < worldSize; i++)
            {
                for (int j = 0; j < sendCnts[i]; j++)
                {
                    struct gameboard boardArray[BOARD_WIDTH + 1];
                    MPI_Recv(&boardArray, 1, MPI_GAMEBOARD_ARRAY, i, 2, MPI_COMM_WORLD, &status);
                    for (int k = 0; k < boardWidth + 1; k++)
                    {
                        gameboardRecvBuffer[displacements[i] + j][k] = boardArray[k];
                    }
                }
            }
        }

        if (rank == 0)
        {
            for (int i = 0; i < currentKnotsCount; i++)
            {
                struct knot *predecessor = getCurrentKnot(&(gameboardRecvBuffer[i][0]));
                predecessor->successors = malloc(sizeof(predecessor->successors) * boardWidth);
                predecessor->successorsCount = 0;
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
                    successor->gameboard = malloc(sizeof(*(successor->gameboard)));
                    *(successor->gameboard) = gameboardRecvBuffer[i][j];
                    calculateHash(successor);
                    int duplicateIndex = pCheckForDuplicate(successor);
                    if (duplicateIndex < 0)
                    {
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
                if (predecessor->successorsCount == 0)
                {
                    free(predecessor->successors);
                    predecessor->successors = NULL;
                }
            }
            free(gameboardRecvBuffer);
            if (nextKnotsCount == 0)
            {
                free(nextKnots);
                nextKnots = NULL;
            }
            currentKnots = NULL;
            currentKnots = nextKnots;
            currentKnotsCount = nextKnotsCount;
            nextKnots = malloc(sizeof(nextKnots[0]) * currentKnotsCount * boardWidth);
            nextKnotsCount = 0;
        }
        if (rank == 0 && !currentKnots)
        {
            treeFinished = 1;
            printf("Tree Finished!\n");
        }
        else
        {
            treeFinished = 0;
        }
        if (rank == 0)
        {
            for (int i = 0; i < worldSize; i++)
            {
                MPI_Send(&treeFinished, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
            }
            for (int i = 0; i < currentKnotsCount; i++)
            {
                printGameboard(currentKnots[i]->gameboard, "Current Knot");
            }
            for (int i = 0; i < nextKnotsCount   ; i++)
            {
                printGameboard(nextKnots[i]->gameboard, "Next Knot");
            }
        }
        else
        {
            int temp = 0;
            MPI_Recv(&temp, 1, MPI_INT, 0, 3, MPI_COMM_WORLD, &status);
            treeFinished = temp;
        }
        printf("Durchgang beendet! Rank %d, Tree Finished = %d\n", rank, treeFinished);
    }
    MPI_Finalize();
}
