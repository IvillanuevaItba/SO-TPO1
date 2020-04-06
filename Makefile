CC=gcc
OBJS = master.o
CFLAGS = -Wall -std=c99 -lpthread -lrt
INCLUDE =
LIBS = master.c

all: slave.o master.o view.o

slave.o: slave.c
	${CC} ${CFLAGS} -o slave.o slave.c

master.o: master.c
	${CC} ${CFLAGS} -o master.o master.c

view.o: view.c
	${CC} ${CFLAGS} -o view.o view.c

clean:
	-rm -f *.o core *.core