#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "createParallelTree.h"
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth, boardHeight;

char invalidInputMessage[42];

struct knot playerKnot;

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
            if (playerKnot.gameboard->lanes[columnIndex][rowIndex] == 1)
            {
                boardPiece = 'O';
            }
            else if (playerKnot.gameboard->lanes[columnIndex][rowIndex] == 2)
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
    while (input < 0 || input < (boardWidth - 1) || !validInput)
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
    result = put(playerKnot.gameboard, getNumberInput());
    while (!result)
    {
        printf("%s", invalidInputMessage);
        fflush(NULL);
        result = put(playerKnot.gameboard, getNumberInput());
    }
    *(playerKnot.gameboard) = *result;
    free(result);
    calculateHash(&playerKnot);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        sprintf(&invalidInputMessage, "Please enter a number between 0 and %d: ", boardWidth - 1);

        struct gameboard emptyBoard = {0};
        emptyBoard.nextPlayer = 1;
        playerKnot.gameboard = &emptyBoard;
        calculateHash(&playerKnot);

        printPlayerPrompt();
        makePlayerTurn();
        printPlayerGameboard();
        printf("Calculating...\n");
    }

    buildParallelTree(&playerKnot);

    if (rank == 0)
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
                    if (!strcmp(playerKnot.gameboardHash, playerKnot.successors[i]->gameboardHash))
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
                //printf("Player Successors: %d\n", playerKnot.successorsCount);
                for (int i = 1; i < playerKnot.successorsCount; i++)
                {
                    if (playerKnot.successors[bestSuccessorIndex]->winPercentage < playerKnot.successors[i]->winPercentage)
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
    }

    MPI_Finalize();
    return 0;
}
