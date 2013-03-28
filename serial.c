#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>    /* File control definitions */
#include <errno.h>    /* Error number definitions */
#include <termios.h>  /* POSIX terminal control definitions */
#include <sys/ioctl.h>
#include <string.h>

#include "serial.h"
#include "osc.h"

/* definitions ********************************************************/
int init_serialport(const char* port)
{
  printf("opening %s\n", port);
  struct termios toptions;
  int fd;

  fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
  //fd = open(port, O_RDWR | O_NOCTTY);
  if (fd == -1)  {
    perror("init_serialport: Unable to open port ");
    return -1;
  }

  if (tcgetattr(fd, &toptions) < 0) {
    perror("init_serialport: Couldn't get term attributes");
    return -1;
  }
  speed_t brate = B115200;
  cfsetispeed(&toptions, brate);
  cfsetospeed(&toptions, brate);

  // 8N1
  toptions.c_cflag &= ~PARENB;
  toptions.c_cflag &= ~CSTOPB;
  toptions.c_cflag &= ~CSIZE;
  toptions.c_cflag |= CS8;
  // no flow control
  toptions.c_cflag &= ~CRTSCTS;

  toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
  toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

  toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
  toptions.c_oflag &= ~OPOST; // make raw

  // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
  toptions.c_cc[VMIN]  = 0;
  toptions.c_cc[VTIME] = 20;

  if( tcsetattr(fd, TCSANOW, &toptions) < 0) {
    perror("init_serialport: Couldn't set term attributes");
    return -1;
  }

  return fd;
}

void pushpedal(const char* str)
{
    int len = strlen(str);
    //printf("****************** writing %d\t: %s\n", len, str);
    write(ser_fd, str, len);
}

