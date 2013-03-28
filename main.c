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
const double REINIT = 1.f;
const double MAX_WAIT = 0.2;
const char * SPORT = "/dev/arduino";

static const char* commands[12] = {
  "undo", "record_or_overdub", "reverse", "redo", "mute", "multiply", "oneshot", "trigger", "undo", "normalspeed", "speedup", "speeddown"
};

unsigned curr_pedal_mode = 3;

unsigned heard_sl = 0;

pthread_t midi_blinker;

/* prototypes *********************************************************/
void do_shutdown(int ret);
void register_sl_cbs();
void unregister_sl_cbs();
int sl_state_cb(const char *path, const char *types, lo_arg **argv, int argc,
                    void *data, void *user_data);
int sl_rate_cb(const char *path, const char *types, lo_arg **argv, int argc,
                    void *data, void *user_data);
int sl_loop_cb(const char *path, const char *types, lo_arg **argv, int argc,
                    void *data, void *user_data);

/* definitions ********************************************************/
void sig_handler(int signum) {
  if (signum==SIGINT) {
    printf("interrupted\n");
    do_shutdown(0);
  }
}

void set_pedal_mode(int mode)
{
  curr_pedal_mode = mode;
  if (mode == 1) {
    pushpedal("-0");
    pushpedal("+1");
  }
  else if (mode ==2 ) {
    pushpedal("+0");
    pushpedal("+1");
  }
  else if (mode ==3 ) {
    // Mode unknown
    pushpedal("-0");
    pushpedal("-1");
  }
  else {
    pushpedal("+0");
    pushpedal("-1");
  }
}

void do_shutdown(int ret)
{
  set_pedal_mode(3);
  pushpedal("!4");
  close(ser_fd);
  printf("shutting down..\n");
  unregister_sl_cbs();
  free((void*)osc_host);
  exit(ret);
}

void get_sl_state()
{
  get_control(-3, "state", osc_cb_state);
}

void register_sl_cbs()
{
  get_sl_state();

  lo_timetag_now(&lastheard);
  unregister_sl_cbs();

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

void sl_control(int press, int command)
{
  if (!heard_sl) return;

  const char* msg = (press == 1 ? "/sl/-3/down" : "/sl/-3/up");
  //printf("%s[%s]\n", commands[command], (press == 1 ? "press" : "up"));
  if (lo_send(sl, msg, "s", commands[command]) == -1) {
    printf("OSC error %d: %s\n", lo_address_errno(sl), lo_address_errstr(sl));
  }
}

void pedal_receive(const int len)
{
  char buffer[2];
  int err = read(ser_fd, &buffer, sizeof(buffer));
  if (err <0)
    perror("error reading\n");
  else if (err ==2) {
    if (buffer[1] == '0') {
      // mode change
      if (buffer[0] == '+' && heard_sl) {
        set_pedal_mode((curr_pedal_mode+1) %3);
      }
    }
    else {
      int press = buffer[0] == '+';
      int diff = 0;
      if (curr_pedal_mode == 1)
        diff = 4;
      else if (curr_pedal_mode == 2)
        diff = 8;
      sl_control(press, diff + buffer[1] - '0' - 1);
    }
  }
}

int main(int argc, char *argv[])
{
  signal(SIGINT, sig_handler);

  msgs = lo_server_new("9961", osc_error);

  sl = lo_address_new(argv[1], "9951");
  osc_host = lo_server_get_url(msgs);
  printf("we are %s\n", osc_host);

  lo_server_add_method(msgs, osc_cb_state, "isf", sl_state_cb, NULL);
  lo_server_add_method(msgs, osc_cb_rate, "isf", sl_rate_cb, NULL);
  lo_server_add_method(msgs, osc_cb_loop, "isf", sl_loop_cb, NULL);

  register_sl_cbs();
  lo_timetag_now(&lastheard);

  fd_set rfds;
  int retval;
  int lo_fd = lo_server_get_socket_fd(msgs);

  /* serial */
  ser_fd = init_serialport(SPORT);
  if (ser_fd < 0) do_shutdown(1);

  /* midi blinken */
  midi_init();
  set_pedal_mode(3);
  pthread_create(&midi_blinker, NULL, midi_blinken, (void*)NULL);

  int max_fd = (lo_fd > ser_fd ? lo_fd : ser_fd) + 1;

  while(1) {
    lo_timetag now;
    lo_timetag_now(&now);
    double diff = lo_timetag_diff(now,lastheard);
    if(diff > REINIT) {
      printf("Finding Sl..\n");
      heard_sl = 0;
  //    set_pedal_mode(3);
      pushpedal("!2");
      register_sl_cbs();
    }
    else if(diff > MAX_WAIT) {
      get_sl_state();
    }

    FD_ZERO(&rfds);
    FD_SET(lo_fd, &rfds);
    FD_SET(ser_fd, &rfds);

    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 100000;

    retval = select(max_fd, &rfds, NULL, NULL, NULL);
    if (retval == -1) {
      printf("select() error\n");
      exit(1);

    } else if (retval > 0) {
      if (FD_ISSET(lo_fd, &rfds)) lo_server_recv_noblock(msgs, 0);
      if (FD_ISSET(ser_fd, &rfds)) pedal_receive(2);
    }
  }

  do_shutdown(0);
}

void on_sl_cb()
{
  if(heard_sl == 0) {
    // causes mode to drop after a select on sl fd
    //set_pedal_mode(0);
    heard_sl = 1;
  }
}

int sl_loop_cb(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
  on_sl_cb();
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
  on_sl_cb();
  lo_timetag_now(&lastheard);
  //printf("%s <- i:%d, s:%s, f:%f\n", path, argv[0]->i, &argv[1]->s, argv[2]->f);
  if (argv[2]->f > 14) {
    return 0;
  }

  if (argv[0]->i>=0) {
    if (argv[2]->f == 10.f)
      midi_led(48+argv[0]->i,1);
    else
      midi_led(48+argv[0]->i,0);

    midi_mode[argv[0]->i] = (int)argv[2]->f;
  }
  else {
    char state = (int)argv[2]->f;
    state+=97;
    char msg[2];
    sprintf(msg, "#%c", state);
    pushpedal(msg);
  }
  return 0;
}
int sl_rate_cb(const char *path, const char *types, lo_arg **argv, int argc,
		 void *data, void *user_data)
{
  on_sl_cb();
  lo_timetag_now(&lastheard);
  //printf("%s <- i:%d, s:%s, f:%f\n", path, argv[0]->i, &argv[1]->s, argv[2]->f);
  char on = (argv[2]->f < 0) ? '+' : '-';
  char msg[2];
  sprintf(msg, "%c4", on);
  pushpedal(msg);
  return 0;
}
