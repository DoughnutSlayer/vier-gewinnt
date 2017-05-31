OBJS = vier-gewinnt.o

vier-gewinnt.o : vier-gewinnt.c
	gcc -c vier-gewinnt.c

.PHONY : clean
clean :
	rm -f $(OBJS)
