CC = gcc
CFLAGS = -Wall -g -DNUM_LOOPS=3
LIBS = -llo -lasound -lpthread
SRCS = main.c serial.c osc.c midi.c
OBJS = $(SRCS:.c=.o)
MAIN = oscdaemon

.PHONY: depend clean

all:    $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
