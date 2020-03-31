CC=gcc
OBJS = master.o
CFLAGS = -Wall -std=c99
INCLUDE =
LIBS = master.c

all: slave.o master.o

slave.o: slave.c
	${CC} ${CFLAGS} -o slave.o slave.c

master.o: master.c
	${CC} ${CFLAGS} -o master.o master.c

clean:
	-rm -f *.o core *.core