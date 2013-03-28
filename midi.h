#ifndef SL_MIDI
#define SL_MIDI
void midi_init();
void midi_close();
void midi_led(int num, int onoff);
void* midi_blinken(void *params);

extern unsigned int midi_blink_step;
extern int midi_blink_rate;
extern int midi_leds[NUM_LOOPS][15][4];
extern int midi_mode[NUM_LOOPS];

#endif /*SL_MIDI */
