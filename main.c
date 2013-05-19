#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include <lo/lo.h>

#include "serial.h"
#include "midi.h"
#include "osc.h"

/* vars ***************************************************************/
const double REINIT = 4.f;
const double MAX_WAIT = 1.f;

static const char* commands[12] = {
  "undo", "record_or_overdub", "reverse", "redo", "mute", "multiply", "oneshot", "trigger", "undo", "normalspeed", "speedup", "speeddown"
};

unsigned mode = 0;

pthread_t midi_blinker;

/* prototypes *********************************************************/
void do_shutdown(int ret);
void register_sl_cbs();
void unregister_sl_cbs();
int sl_ping_cb(const char *path, const char *types, lo_arg **argv, int argc,
                    void *data, void *user_data);
int sl_state_cb(const char *path, const char *types, lo_arg **argv, int argc,
                    void *data, void *user_data);
int sl_rate_cb(const char *path, const char *types, lo_arg **argv, int argc,
                    void *data, void *user_data);
int sl_loop_cb(const char *path, const char *types, lo_arg **argv, int argc,
                    void *data, void *user_data);
void update_mode();

/* definitions ********************************************************/
void sig_handler(int signum) {
  if (signum==SIGINT) {
    printf("interrupted\n");
    do_shutdown(0);
  }
}

void update_mode()
{
  printf("setting mode\n");
  if (mode == 1) {
    send_serial("-0");
    send_serial("+1");
  }
  else if (mode ==2 ) {
    send_serial("+0");
    send_serial("+1");
  }
  else {
    send_serial("+0");
    send_serial("-1");
  }
}

void do_shutdown(int ret)
{
  //send_serial("!\n");
  close(ser_fd);
  printf("shutting down..\n");
  unregister_sl_cbs();
  printf("stopping msgs thread\n");
  //if (msgs) lo_server_thread_stop(msgs);
  printf("freeing msgs thread\n");
  //if (msgs) lo_server_thread_free(msgs);
  free((void*)osc_host);
  exit(ret);
}

void register_sl_cbs()
{
  lo_timetag_now(&lastheard);
  unregister_sl_cbs();

  lo_server_add_method(msgs, "/ping", "ssi", sl_ping_cb, NULL);
  lo_server_add_method(msgs, osc_cb_state, "isf", sl_state_cb, NULL);
  lo_server_add_method(msgs, osc_cb_rate, "isf", sl_rate_cb, NULL);
  lo_server_add_method(msgs, osc_cb_loop, "isf", sl_loop_cb, NULL);
  int i;
  reg_cb(-3,"state",osc_cb_state);
  reg_cb(-3,"rate_output",osc_cb_rate);
  reg_cb(-2,"selected_loop_num", osc_cb_loop);
  for (i=0;i<NUM_LOOPS;i++) {
    reg_cb(i,"state\0",osc_cb_state);
    reg_cb(i,"rate_output\0",osc_cb_rate);
  }
}

void unregister_sl_cbs()
{
  int i;
  unreg_cb(-3,"state\0",osc_cb_state);
  unreg_cb(-3,"rate_output\0",osc_cb_rate);
  unreg_cb(-2,"selected_loop_num", osc_cb_loop);
  for (i=0;i<NUM_LOOPS;i++) {
    unreg_cb(i,"state",osc_cb_state);
    unreg_cb(i,"rate_output",osc_cb_rate);
  }
}

void send_ping()
{
  if (lo_send(sl, "/ping", "ss", osc_host, "/ping") == -1) {
    printf("OSC error %d: %s\n", lo_address_errno(sl), lo_address_errstr(sl));
    do_shutdown(1);
  }
}

void send_command(int press, int command)
{
  const char* msg = (press == 1 ? "/sl/-3/down" : "/sl/-3/up");
  printf("sending %s %s\n", msg, commands[command]);
  if (lo_send(sl, msg, "s", commands[command]) == -1) {
    printf("OSC error %d: %s\n", lo_address_errno(sl), lo_address_errstr(sl));
  }
}

void read_serial(const int len)
{
  char buffer[2];
  int err = read(ser_fd, &buffer, sizeof(buffer));
  if (err <0)
    perror("error reading\n");
  else if (err ==2) {
    if (buffer[1] == '0') {
      // mode change
      if (buffer[0] == '+') {
        mode = (mode+1) %3;
        update_mode();
        printf("mode %u\n", mode);
      }
    }
    else {
      int press = buffer[0] == '+';
      int diff = 0;
      if (mode == 1)
        diff = 4;
      else if (mode == 2)
        diff = 8;
      send_command(press, diff + buffer[1] - '0' - 1);
    }
  }
}

int main(int argc, char *argv[])
{
  signal(SIGINT, sig_handler);

  ser_fd = init_serialport("/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A4012YJD-if00-port0");
  if (ser_fd < 0) do_shutdown(1);

  msgs = lo_server_new("9961", osc_error);

  sl = lo_address_new(argv[1], "9951");
  osc_host = lo_server_get_url(msgs);
  printf("we are %s\n", osc_host);

  register_sl_cbs();
  lo_timetag_now(&lastheard);

  fd_set rfds;
  int retval;
  int lo_fd = lo_server_get_socket_fd(msgs);

  /* midi blinken */
  midi_init();
  update_mode();
  pthread_create(&midi_blinker, NULL, midi_blinken, (void*)NULL);

  int max_fd = (lo_fd > ser_fd ? lo_fd : ser_fd) + 1;

  while(1) {
    lo_timetag now;
    lo_timetag_now(&now);
    double diff = lo_timetag_diff(now,lastheard);
    if(diff > REINIT) {
      printf("registering\n");
      register_sl_cbs();
    }
    else if(diff > MAX_WAIT) {
      printf("not heard\n");
      send_ping();
    }

    FD_ZERO(&rfds);
    FD_SET(lo_fd, &rfds);
    FD_SET(ser_fd, &rfds);
    retval = select(max_fd, &rfds, NULL, NULL, NULL); /* no timeout */
    if (retval == -1) {
      printf("select() error\n");
      exit(1);

    } else if (retval > 0) {
      if (FD_ISSET(lo_fd, &rfds)) lo_server_recv_noblock(msgs, 0);
      if (FD_ISSET(ser_fd, &rfds)) read_serial(2);
    }
  }

  do_shutdown(0);
}

int sl_ping_cb(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
  lo_timetag_now(&lastheard);
  return 0;
}

int sl_loop_cb(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
  lo_timetag_now(&lastheard);
  int i;
  for (i=0;i<NUM_LOOPS;i++) {
    midi_led(32+i, i == (int)argv[2]->f);
  }
  return 0;
}

int sl_state_cb(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
  lo_timetag_now(&lastheard);
  printf("%s <- i:%d, s:%s, f:%f\n", path, argv[0]->i, &argv[1]->s, argv[2]->f);
  if (argv[2]->f > 14)
    return 0;

  if (argv[0]->i>=0) {
    if (argv[2]->f == 10.f)
      midi_led(48+argv[0]->i,1);
    else
      midi_led(48+argv[0]->i,0);

    midi_mode[argv[0]->i] = (int)argv[2]->f;
  }
  else {
    char mode = (int)argv[2]->f;
    mode+=97;
    char msg[2];
    sprintf(msg, "#%c", mode);
    send_serial(msg);
  }
  return 0;
}
int sl_rate_cb(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
  lo_timetag_now(&lastheard);
  printf("%s <- i:%d, s:%s, f:%f\n", path, argv[0]->i, &argv[1]->s, argv[2]->f);
  char on = (argv[2]->f < 0) ? '+' : '-';
  char msg[2];
  sprintf(msg, "%c4", on);
  send_serial(msg);
  return 0;
}
