ifndef CC
	CC = gcc
endif
CFLAGS = -Wall
DEPS = gameboard.h
OBJS = gameboard.o

gameboard.o : gameboard.c $(DEPS)
	$(CC) $(CFLAGS) -c gameboard.c

.PHONY : clean
clean :
	rm -f $(OBJS)
