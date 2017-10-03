ifndef BOARD_WIDTH
	BOARD_WIDTH = 4
endif

ifndef BOARD_HEIGHT
	BOARD_HEIGHT = 4
endif

CC = mpicc

CFLAGS = -Wall -std=c99 -DBOARD_WIDTH=$(BOARD_WIDTH) -DBOARD_HEIGHT=$(BOARD_HEIGHT)
DEPS = createParallelTree.h createSequentialTree.h gameboard.h knot.h calculateSequentialWinPercentage.h
OBJS = createParallelTree.o createSequentialTree.o gameboard.o knot.o calculateSequentialWinPercentage.o vier-gewinnt.o

vier-gewinnt : $(OBJS)
	$(CC) $(CFLAGS) -lm -o vier-gewinnt $(OBJS)

calculateSequentialWinPercentage.o : calculateSequentialWinPercentage.c $(DEPS)
	$(CC) $(CFLAGS) -c calculateSequentialWinPercentage.c

createParallelTree.o : createParallelTree.c $(DEPS)
	$(CC) $(CFLAGS) -c createParallelTree.c

createSequentialTree.o : createSequentialTree.c $(DEPS)
	$(CC) $(CFLAGS) -c createSequentialTree.c

gameboard.o : gameboard.c $(DEPS)
	$(CC) $(CFLAGS) -c gameboard.c

knot.o : knot.c $(DEPS)
	$(CC) $(CFLAGS) -c knot.c

vier-gewinnt.o : vier-gewinnt.c $(DEPS)
	$(CC) $(CFLAGS) -c vier-gewinnt.c

.PHONY : all
all :
	make $(OBJS)

.PHONY : clean
clean :
	rm -f $(OBJS) vier-gewinnt vier-gewinnt.exe
