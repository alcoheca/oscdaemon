#ifndef SL_SERIAL
#define SL_SERIAL

/* globals ************************************************************/
int ser_fd;

/* prototypes *********************************************************/
int  init_serialport(const char* port);
int send_serial(const char* msg);
int read_serial(const int len);

#endif /* SL_SERIAL */
