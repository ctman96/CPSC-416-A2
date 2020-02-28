all: node


CLIBS=-pthread
CC=gcc
CPPFLAGS=
CFLAGS=-g

NODEOBJS=node.o logger.o state_machine.o

node: $(NODEOBJS)
	$(CC) -o node $(NODEOBJS)  $(CLIBS)



clean:
	rm -f *.o
	rm -f node

