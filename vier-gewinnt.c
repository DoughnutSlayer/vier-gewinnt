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

void makePlayerTurn()
{
    struct gameboard *result = NULL;
    result = put(playerGameboard, getNumberInput());
    while (!result)
    {
        printf("%s", invalidInputMessage);
        fflush(NULL);
        result = put(playerGameboard, getNumberInput());
    }
    *playerGameboard = *result;
    calculateHash(playerGameboard);
    free(result);
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
    }

    buildParallelTree(playerKnot, playerGameboard);

    /*if (rank == 0)
    {
        printf("Resume\n");
        while (!playerKnot.gameboard->isWonBy)
        {
            if (playerKnot.gameboard->nextPlayer == 1)
            {
                printPlayerPrompt();
                makePlayerTurn();
                for (int i = 0; i < playerKnot.successorsCount; i++)
                {
                    if (!strcmp(playerKnot.gameboard->hash,
                                playerKnot.successors[i]->gameboard->hash))
                    {
                        playerKnot = *(playerKnot.successors[i]);
                        break;
                    }
                }
            }
            else
            {
                printPlayerGameboard();
                int bestSuccessorIndex = 0;
                // printf("Player Successors: %d\n",
                // playerKnot.successorsCount);
                for (int i = 1; i < playerKnot.successorsCount; i++)
                {
                    if (playerKnot.successors[bestSuccessorIndex]->winPercentage
                        < playerKnot.successors[i]->winPercentage)
                    {
                        bestSuccessorIndex = i;
                    }
                }
                playerKnot = *(playerKnot.successors[bestSuccessorIndex]);
            }
        }
        printPlayerGameboard();
        if (playerKnot.gameboard->isWonBy == 1)
        {
            printf("You Win!\n");
        }
        else if (playerKnot.gameboard->isWonBy == 2)
        {
            printf("You Lose!\n");
        }
        else if (playerKnot.gameboard->isWonBy == 2)
        {
            printf("It's a draw!\n");
        }
    }*/

    MPI_Finalize();
    return 0;
}
