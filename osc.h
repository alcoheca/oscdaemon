#ifndef SL_OSC
#define SL_OSC
#include <lo/lo.h>

lo_server_thread msgs;
lo_address sl;
lo_timetag lastheard;

extern const char* osc_host;
extern const char* osc_cb_state;
extern const char* osc_cb_rate;
extern const char* osc_cb_loop;

void osc_error(int num, const char *msg, const char *where);
int  osc_generic_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data);

void get_control(int loop, const char* control, const char* cb_addr);

void reg_cb(int loop, const char* monitor, const char* cb_addr);
void unreg_cb(int loop, const char* monitor, const char* cb_addr);

#endif /* SL_OSC */
