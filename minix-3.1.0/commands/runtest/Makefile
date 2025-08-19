# Makefile for the 'runtest' command

PROG = runtest
CFLAGS = -O -D_MINIX -D_POSIX_SOURCE
LDFLAGS = -i
CC = exec cc

# Object files for the main runtest command
OBJS = runtest.o

# The 'all' target builds both runtest and the worker program
all: $(PROG) worker

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

worker: worker.o
	$(CC) $(LDFLAGS) -o $@ worker.o

# The 'install' target copies the command to /usr/bin
install: /usr/bin/$(PROG)

/usr/bin/$(PROG): $(PROG)
	install -cs -o bin $? $@

# The worker program does not need to be installed, as runtest will
# execute it from the current directory during tests.

clean:
	rm -f $(PROG) worker *.o *.bak core