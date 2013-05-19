#ifndef SL_SERIAL
#define SL_SERIAL

/* globals ************************************************************/
int ser_fd;

/* prototypes *********************************************************/
int  init_serialport(const char* port);
void send_serial(const char* msg);
void read_serial(const int len);

#endif /* SL_SERIAL */
