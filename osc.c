#include "osc.h"

#include <stdio.h>

const char* osc_host = NULL;
const char* osc_cb_state = "/ard/state";
const char* osc_cb_rate = "/ard/rate";

void osc_error(int num, const char *msg, const char *where)
{
    printf("liblo server error %d in %s: %s\n", num, where, msg);
}

/* catch any incoming messages and display them. returning 1 means that the
 * message has not been fully handled and the server should try other methods */
int osc_generic_handler(const char *path, const char *types, lo_arg **argv,
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

void reg_cb(int loop, const char* monitor, const char* cb_addr)
{
  char addr[27];
  sprintf(addr, "/sl/%d/register_auto_update", loop);
  if (lo_send(sl, addr, "siss", monitor, 200, osc_host, cb_addr) == -1) {
    printf("OSC error %d: %s\n", lo_address_errno(sl), lo_address_errstr(sl));
  }
}

void unreg_cb(int loop, const char* monitor, const char* cb_addr)
{
  char addr[29];
  sprintf(addr, "/sl/%d/unregister_auto_update", loop);
  if (lo_send(sl, addr, "siss", monitor, 200, osc_host, cb_addr) == -1) {
    printf("OSC error %d: %s\n", lo_address_errno(sl), lo_address_errstr(sl));
  }
}
