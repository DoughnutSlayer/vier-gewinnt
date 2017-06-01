ifndef BOARD_WIDTH
	BOARD_WIDTH = 4
endif

ifndef BOARD_HEIGHT
	BOARD_HEIGHT = 4
endif

ifndef CC
	CC = gcc
endif

CFLAGS = -Wall -DBOARD_WIDTH=$(BOARD_WIDTH) -DBOARD_HEIGHT=$(BOARD_HEIGHT)
DEPS = gameboard.h
OBJS = gameboard.o

gameboard.o : gameboard.c $(DEPS)
	$(CC) $(CFLAGS) -c gameboard.c

.PHONY : clean
clean :
	rm -f $(OBJS)
