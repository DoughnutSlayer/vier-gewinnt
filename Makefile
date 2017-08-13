ifndef BOARD_WIDTH
	BOARD_WIDTH = 4
endif

ifndef BOARD_HEIGHT
	BOARD_HEIGHT = 4
endif

ifndef CC
	CC = gcc
endif

CFLAGS = -Wall -std=c99 -DBOARD_WIDTH=$(BOARD_WIDTH) -DBOARD_HEIGHT=$(BOARD_HEIGHT)
DEPS = createSequentialTree.h gameboard.h knot.h
OBJS = calculateSequentialWinPercentage.o createSequentialTree.o gameboard.o knot.o

calculateSequentialWinPercentage.o : calculateSequentialWinPercentage.c $(DEPS)
	$(CC) $(CFLAGS) -c calculateSequentialWinPercentage.c

createSequentialTree.o : createSequentialTree.c $(DEPS)
	$(CC) $(CFLAGS) -c createSequentialTree.c

gameboard.o : gameboard.c $(DEPS)
	$(CC) $(CFLAGS) -c gameboard.c

knot.o : knot.c $(DEPS)
	$(CC) $(CFLAGS) -c knot.c

.PHONY : all
all :
	make $(OBJS)

.PHONY : clean
clean :
	rm -f $(OBJS)
