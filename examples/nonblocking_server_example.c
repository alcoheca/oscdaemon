/*
 *  nonblocking_server_example.c
 *
 *  This code demonstrates two methods of monitoring both an lo_server
 *  and other I/O from a single thread.
 *
 *  Copyright (C) 2004 Steve Harris, Uwe Koloska
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: nonblocking_server_example.c,v 1.3 2005/12/21 15:58:55 nhumfrey Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <strings.h>
#include <unistd.h>

#include "lo/lo.h"

int done = 0;

void error(int num, const char *m, const char *path);

int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data);

int state_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data);

void read_stdin(void);

int main()
{
    int lo_fd;
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* start a new server on port 7770 */
    lo_server s = lo_server_new("9961", error);

    /* add method that will match any path and args */
    lo_server_add_method(s, NULL, NULL, generic_handler, NULL);

    /* add method that will match the path /foo/bar, with two numbers, coerced
     * to float and int */
    lo_server_add_method(s, "/ard/state", "isf", state_handler, NULL);

    /* register callbacks */
    lo_address sl = lo_address_new("ragtop", "9951");
    if (lo_send(sl, "/sl/-1/register_auto_update", "siss", "state", 200, "osc.udp://ragtop:9961/", "/ard/state") == -1) {
      printf("OSC error %d: %s\n", lo_address_errno(sl), lo_address_errstr(sl));
    }

    /* get the file descriptor of the server socket, if supported */
    lo_fd = lo_server_get_socket_fd(s);

    /* select() on lo_server fd is supported, so we'll use select()
     * to watch both stdin and the lo_server fd. */

    do {

        FD_ZERO(&rfds);
        FD_SET(lo_fd, &rfds);

        retval = select(lo_fd + 1, &rfds, NULL, NULL, NULL); /* no timeout */

        if (retval == -1) {

            printf("select() error\n");
            exit(1);

        } else if (retval > 0) {

            if (FD_ISSET(0, &rfds)) {

                read_stdin();

            }
            if (FD_ISSET(lo_fd, &rfds)) {

                lo_server_recv_noblock(s, 0);

            }
        }

    } while (!done);

    return 0;
}

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
    int i;

    printf("path: <%s>\n", path);
    for (i=0; i<argc; i++) {
	printf("arg %d '%c' ", i, types[i]);
	lo_arg_pp(types[i], argv[i]);
	printf("\n");
    }
    printf("\n");
    fflush(stdout);

    return 1;
}

int state_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    /* example showing pulling the argument values out of the argv array */
    printf("about to crash\n");
    printf("%s <- i:%d, s:%s, f:%f\n\n", path, argv[0]->i, argv[1]->s, argv[2]->f);
    fflush(stdout);

    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
    done = 1;
    printf("quiting\n\n");

    return 0;
}

void read_stdin(void)
{
    char buf[256];
    int len = read(0, buf, 256);
    if (len > 0) {
        printf("stdin: ");
        fwrite(buf, len, 1, stdout);
        printf("\n");
        fflush(stdout);
    }
}

/* vi:set ts=8 sts=4 sw=4: */
