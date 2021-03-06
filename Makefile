CC = gcc
CFLAGS = -Wall -g -DNUM_LOOPS=3
LIBS = -llo -lasound -lpthread
SRCS = main.c serial.c osc.c midi.c
OBJS = $(SRCS:.c=.o)
MAIN = oscdaemon

.PHONY: clean

all:    $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

install: all
	mkdir -p ${DESTDIR}/usr/local/bin
	install -m 0755 oscdaemon $(DESTDIR)/usr/local/bin/
	install -m 0755 oscdaemon.service $(DESTDIR)/usr/local/bin/
	install -m 0755 oscdaemon.launch $(DESTDIR)/usr/local/bin/
	mkdir -p ${DESTDIR}/etc/udev/rules.d
	install -m 0644 81-oscdaemon.rules $(DESTDIR)/etc/udev/rules.d/
