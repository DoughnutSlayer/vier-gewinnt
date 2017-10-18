#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "createParallelTree.h"
#include "gameboard.h"
#include "knot.h"
#include "mpi.h"

extern const int boardWidth, boardHeight;

char invalidInputMessage[42];

struct knot *playerKnot;
struct gameboard *playerGameboard;

int turnIndex = -1; // TODO: Give turnIndex to createParallelTree;

void printPlayerGameboard()
{
    printf(" ");
    for (int i = 0; i < boardWidth; i++)
    {
        printf(" %d  ", i);
    }
    printf("\n");

    printf("┌───");
    for (int i = 1; i < boardWidth; i++)
    {
        printf("┬───");
    }
    printf("┐\n");

    for (int rowIndex = 0; rowIndex < boardHeight; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < boardWidth; columnIndex++)
        {
            char boardPiece = ' ';
            if (playerGameboard->lanes[columnIndex][rowIndex] == 1)
            {
                boardPiece = 'O';
            }
            else if (playerGameboard->lanes[columnIndex][rowIndex] == 2)
            {
                boardPiece = 'X';
            }
            printf("│ %c ", boardPiece);
        }
        printf("│\n");

        if (rowIndex == boardHeight - 1)
        {
            break;
        }
        printf("├───");
        for (int columnIndex = 1; columnIndex < boardWidth; columnIndex++)
        {
            printf("┼───");
        }
        printf("┤\n");
    }

    printf("└───");
    for (int i = 1; i < boardWidth; i++)
    {
        printf("┴───");
    }
    printf("┘\n");
}

void printPlayerPrompt()
{
    printPlayerGameboard();
    printf("Enter where to put the next 'O': ");
    fflush(NULL);
}

int getNumberInput()
{
    int validInput = 0;
    int input;
    validInput = scanf("%d", &input);
    while (input < 0 || input > (boardWidth - 1) || !validInput)
    {
        if (!validInput)
        {
            scanf("%*s");
        }
        printf("%s", invalidInputMessage);
        fflush(NULL);
        validInput = scanf("%d", &input);
    }
    return input;
}

int makePlayerTurn()
{
    struct gameboard *newGameboard = NULL;
    int input = getNumberInput();
    newGameboard = put(playerGameboard, input);
    while (!newGameboard)
    {
        printf("%s", invalidInputMessage);
        fflush(NULL);
        input = getNumberInput();
        newGameboard = put(playerGameboard, input);
    }
    *playerGameboard = *newGameboard;
    calculateHash(playerGameboard);
    free(newGameboard);
    turnIndex++;
    return input;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, worldSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    if (worldSize < 2)
    {
        printf("Please start this program with at least two processes, for "
               "example by using mpirun.\n");
        MPI_Finalize();
        return 0;
    }

    struct knot **(turns[BOARD_WIDTH * BOARD_HEIGHT]);
    struct gameboard *playerGameboardCopy =
      malloc(sizeof(*playerGameboardCopy));
    if (rank == 0)
    {
        sprintf(invalidInputMessage, "Please enter a number between 0 and %d: ",
                boardWidth - 1);

        struct knot initialKnot = {.winPercentage = 0};
        playerKnot = malloc(sizeof(*playerKnot));
        *playerKnot = initialKnot;

        struct gameboard initialGameboard = {.nextPlayer = 1};
        playerGameboard = malloc(sizeof(*playerGameboard));
        *playerGameboard = initialGameboard;
        calculateHash(playerGameboard);

        printPlayerPrompt();
        makePlayerTurn();
        printPlayerGameboard();
        printf("Calculating...\n");

        *playerGameboardCopy = *playerGameboard;
    }

    buildParallelTree(playerKnot, playerGameboard, &turns);

    if (rank == 0)
    {
        printf("Resume\n");

        playerGameboard = playerGameboardCopy;
        while (!playerGameboard->isWonBy)
        {
            if (playerGameboard->nextPlayer == 1)
            {
                printPlayerPrompt();
                int input = makePlayerTurn();
                int nextKnotIndex = playerKnot->successorIndices[input];
                playerKnot = turns[turnIndex][nextKnotIndex];
            }
            else
            {
                printPlayerGameboard();
                int bestTurn = 0;
                double bestWinpercentage = 0;
                for (int i = 0; i < boardWidth; i++)
                {
                    int successorIndex = playerKnot->successorIndices[i];
                    if (successorIndex < 0)
                    {
                        continue;
                    }

                    double successorWinPercentage =
                      turns[turnIndex + 1][successorIndex]->winPercentage;
                    if (bestWinpercentage <= successorWinPercentage)
                    {
                        bestTurn = i;
                        bestWinpercentage = successorWinPercentage;
                    }
                }
                playerKnot =
                  turns[turnIndex + 1][playerKnot->successorIndices[bestTurn]];
                struct gameboard *newGameboard;
                newGameboard = put(playerGameboard, bestTurn);
                *playerGameboard = *newGameboard;
                free(newGameboard);
                turnIndex++;
            }
        }
        printPlayerGameboard();
        if (playerGameboard->isWonBy == 1)
        {
            printf("You Win!\n");
        }
        else if (playerGameboard->isWonBy == 2)
        {
            printf("You Lose!\n");
        }
        else if (playerGameboard->isWonBy == 3)
        {
            printf("It's a draw!\n");
        }
    }

    MPI_Finalize();
    return 0;
}
