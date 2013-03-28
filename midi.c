#include <alsa/asoundlib.h>
#include "midi.h"

const char* KONTROLADDR = "20:0";
const int KONTROLCHAN = 0;

snd_seq_t *seq_handle;
int out_port;
snd_seq_addr_t seq_addr;
snd_seq_event_t ev;

unsigned int step = 0;
int midi_blink_rate = 125;
int midi_mode[3] = {0,0,0};

int midi_leds[NUM_LOOPS][15][4] = {
  { {0, 0, 0, 0}, // off
    {1, 0, 0, 0}, // WaitStart
    {1, 0, 1, 0}, // Recording
    {0, 0, 0, 0}, // WaitStop
    {1, 1, 1, 1}, // Playing
    {1, 0, 1, 0}, // Overdubbing
    {1, 1, 1, 0}, // Multiplying
    {0, 0, 0, 0}, // Inserting
    {0, 0, 0, 0}, // Replacing
    {0, 0, 0, 0}, // Delay
    {0, 0, 0, 1}, // Muted
    {0, 0, 0, 0}, // Scratching
    {1, 1, 1, 0}, // OneShot
    {0, 0, 0, 0}, // Substitue
    {0, 0, 0, 0}  // Paused
  },
  { {0, 0, 0, 0}, // off
    {1, 0, 0, 0}, // WaitStart
    {1, 0, 1, 0}, // Recording
    {0, 0, 0, 0}, // WaitStop
    {1, 1, 1, 1}, // Playing
    {1, 0, 1, 0}, // Overdubbing
    {1, 1, 1, 0}, // Multiplying
    {0, 0, 0, 0}, // Inserting
    {0, 0, 0, 0}, // Replacing
    {0, 0, 0, 0}, // Delay
    {0, 0, 0, 1}, // Muted
    {0, 0, 0, 0}, // Scratching
    {1, 1, 1, 0}, // OneShot
    {0, 0, 0, 0}, // Substitue
    {0, 0, 0, 0}  // Paused
  },
  { {0, 0, 0, 0}, // off
    {1, 0, 0, 0}, // WaitStart
    {1, 0, 1, 0}, // Recording
    {0, 0, 0, 0}, // WaitStop
    {1, 1, 1, 1}, // Playing
    {1, 0, 1, 0}, // Overdubbing
    {1, 1, 1, 0}, // Multiplying
    {0, 0, 0, 0}, // Inserting
    {0, 0, 0, 0}, // Replacing
    {0, 0, 0, 0}, // Delay
    {0, 0, 0, 1}, // Muted
    {0, 0, 0, 0}, // Scratching
    {1, 1, 1, 0}, // OneShot
    {0, 0, 0, 0}, // Substitue
    {0, 0, 0, 0}  // Paused
  }
};

void midi_init()
{
  if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
    fprintf(stderr, "Error opening ALSA sequencer.\n");
    return;
  }
  snd_seq_set_client_name(seq_handle, "MidiSend");
  if ((out_port = snd_seq_create_simple_port(seq_handle, "MidiSend",
                  SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                  SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
    printf("error opening port\n");
    return;
  }
  snd_seq_parse_address(seq_handle, &seq_addr, KONTROLADDR);
  snd_seq_connect_to(seq_handle, out_port, seq_addr.client, seq_addr.port);
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_source(&ev, out_port);
}

void midi_close()
{
  snd_seq_close(seq_handle);
}

void update_midi_leds()
{
  int i;
  for (i=0;i<NUM_LOOPS;i++) {
    midi_led(64+i,midi_leds[i][midi_mode[i]][step]);
  }
  step = (step+1) %4;
}

void* midi_blinken(void *params)
{
  step = 0;
  do {
    update_midi_leds();
    poll(0, 0, midi_blink_rate);
  } while (1);
}

void midi_led(int num, int onoff)
{
  ev.type = onoff ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
  ev.data.note.note = num;
  ev.data.note.velocity = 127;
  snd_seq_event_output_direct(seq_handle, &ev);
}
