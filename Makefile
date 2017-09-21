ifndef BOARD_WIDTH
	BOARD_WIDTH = 4
endif

ifndef BOARD_HEIGHT
	BOARD_HEIGHT = 4
endif

CC = mpicc

CFLAGS = -Wall -g -std=c99 -DBOARD_WIDTH=$(BOARD_WIDTH) -DBOARD_HEIGHT=$(BOARD_HEIGHT)
DEPS = createParallelTree.h createSequentialTree.h gameboard.h knot.h testHelp.h calculateSequentialWinPercentage.h
OBJS = createParallelTree.o createSequentialTree.o gameboard.o knot.o testHelp.o calculateSequentialWinPercentage.o testCreateParallelTree.o

testCreateParallelTree : $(OBJS)
	$(CC) $(CFLAGS) -o testCreateParallelTree $(OBJS)

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

testHelp.o : testHelp.c $(DEPS)
	$(CC) $(CFLAGS) -c testHelp.c

testCreateParallelTree.o : testCreateParallelTree.c $(DEPS)
	$(CC) $(CFLAGS) -c testCreateParallelTree.c

.PHONY : all
all :
	make $(OBJS)

.PHONY : clean
clean :
	rm -f $(OBJS) testCreateParallelTree testCreateParallelTree.exe
