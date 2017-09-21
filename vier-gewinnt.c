#include <stdio.h>
#include <stdlib.h>
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
    while (validInput <= 0)
    {
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
    }
    buildParallelTree(&playerKnot);

    MPI_Finalize();
    return 0;
}
