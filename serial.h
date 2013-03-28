#ifndef SL_SERIAL
#define SL_SERIAL

/* globals ************************************************************/
int ser_fd;

/* prototypes *********************************************************/
int  init_serialport(const char* port);
void pushpedal(const char* msg);
void getpedal(const int len);

#endif /* SL_SERIAL */
