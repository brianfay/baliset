#include <Bela.h>
#include <digital_gpio_mapping.h>
#include <cmath>
#include <csignal>
#include "protopatch.h"

patch *p;

int debounce_frames;

typedef struct
{
  int pin;
  uint64_t last_frame_sampled;
  int state;
} Button;

Button btn1, btn2, btn3;

/**
 * Returns 1.0 if the button is down, 0.0 if it's up.
 * digitalRead either returns 0 or some positive int; this maps all positive numbers to 1.0
 * does some debouncing - when a transition is detected, stops listening until debounce_frames (global var) have passed
 * this should protect against instability when pushing or releasing a button.
 * But I guess it doesn't help with other potential issues, like electrical interference
 */
float getButtonValue(BelaContext *context, int frame, Button *btn)
{
  int old_state = btn->state;
  if ((context->audioFramesElapsed - btn->last_frame_sampled) > debounce_frames){
    btn->state = digitalRead(context, frame, btn->pin);
    if(old_state == 0 and btn->state > 0){
      btn->last_frame_sampled = context->audioFramesElapsed;
      return 1.0;
    }
    if(old_state > 0 && btn->state == 0){
      btn->last_frame_sampled = context->audioFramesElapsed;
      return 0.0;
    }
  }

  if(btn->state == 0){return 0.0;}
  return 1.0;
}

bool setup(BelaContext *context, void *userData)
{
  debounce_frames = ceilf(0.05 * context->audioSampleRate); //50 ms delay, 2205 samples at CD rate

  //hardcoding three button inputs for now; would be interesting to do something more dynamic
  pinMode(context, 0, P9_12, INPUT);
  pinMode(context, 0, P9_14, INPUT);
  pinMode(context, 0, P9_16, INPUT);
  btn1.pin = P9_12;
  btn1.last_frame_sampled = 0;
  btn1.state = 0;
  btn2.pin = P9_14;
  btn2.last_frame_sampled = 0;
  btn2.state = 0;
  btn3.pin = P9_16;
  btn3.last_frame_sampled = 0;
  btn3.state = 0;

  audio_options audio_opts = {.buf_size=context->audioFrames, .sample_rate=context->audioSampleRate,
                              .hw_in_channels=context->audioInChannels, .hw_out_channels=context->audioOutChannels,
                              .digital_channels=3, .digital_frames=context->audioFrames}; //using audioFrames buffer size instead of digital
  p = new_patch(audio_opts);

  /* guitar delay
  node *adc = new_adc(p);
  node *delay_l = new_delay(p);
  node *delay_r = new_delay(p);
  node *dac = new_dac(p);

  add_node(p, adc);
  add_node(p, delay_l);
  add_node(p, delay_r);
  add_node(p, dac);

  set_control(delay_l, "delay_time", 0.97);
  set_control(delay_r, "delay_time", 1.44);

  pp_connect(p, adc->id, 0, delay_l->id, 0);
  pp_connect(p, adc->id, 0, delay_r->id, 0);
  pp_connect(p, delay_l->id, 0, dac->id, 0);
  pp_connect(p, delay_r->id, 0, dac->id, 1);
  */

  //sin multiply with button press
  node *osc = new_sin_osc(p);
  node *osc2 = new_sin_osc(p);
  node *osc3 = new_sin_osc(p);
  node *mul = new_mul(p);
  node *mul2 = new_mul(p);
  node *mul3 = new_mul(p);
  node *digiread = new_digiread(p);
  node *dac = new_dac(p);
  add_node(p, osc);
  add_node(p, osc2);
  add_node(p, osc3);
  add_node(p, mul);
  add_node(p, mul2);
  add_node(p, mul3);
  add_node(p, digiread);
  add_node(p, dac);

  set_control(osc, "freq", 330.0);
  set_control(osc2, "freq", 440.0);
  set_control(osc3, "freq", 660.0);

  pp_connect(p, osc->id, 0, mul->id, 0);
  pp_connect(p, osc2->id, 0, mul2->id, 0);
  pp_connect(p, osc3->id, 0, mul3->id, 0);
  pp_connect(p, digiread->id, 0, mul->id, 1);
  pp_connect(p, digiread->id, 1, mul2->id, 1);
  pp_connect(p, digiread->id, 2, mul3->id, 1);
  pp_connect(p, mul->id, 0, dac->id, 0);
  pp_connect(p, mul->id, 0, dac->id, 1);
  pp_connect(p, mul2->id, 0, dac->id, 0);
  pp_connect(p, mul2->id, 0, dac->id, 1);
  pp_connect(p, mul3->id, 0, dac->id, 0);
  pp_connect(p, mul3->id, 0, dac->id, 1);
  //pp_connect(p, osc->id, 0, dac->id, 0);
  //pp_connect(p, osc->id, 0, dac->id, 1);

  sort_patch(p);
	return true;
}

void render(BelaContext *context, void *userData)
{
  const float *in = (float*)context->audioIn;

  //TODO set backend_data with info needed by the nodes

  //read digital inputs
  //upsample
  if(context->digitalFrames == context->audioFrames) {
    for(int i=0; i < context->digitalFrames; i++) {
      p->digital_outlets[0].buf[i] = getButtonValue(context, i, &btn1);
      p->digital_outlets[1].buf[i] = getButtonValue(context, i, &btn2);
      p->digital_outlets[2].buf[i] = getButtonValue(context, i, &btn3);
    }
  } else if (context->digitalFrames < context->audioFrames) {
    //assuming both are powers of two
    //upsampling with zero-order hold, would probably be a bad idea for audio but I think it's okay here
    unsigned int shift_amount = 1;
    while(context->audioFrames >> shift_amount != context->digitalFrames) shift_amount++;
    for(int i=0; i < context->audioFrames; i++){
      //calling getButtonValue for each of these is probably more expensive than this needs to be
      //but I think it's fine
      p->digital_outlets[0].buf[i] = getButtonValue(context, i >> shift_amount, &btn1);
      p->digital_outlets[0].buf[i] = getButtonValue(context, i >> shift_amount, &btn2);
      p->digital_outlets[0].buf[i] = getButtonValue(context, i >> shift_amount, &btn3);
    }
    //not handling the case where digitalFrames > audioFrames because that shouldn't happen
  }

  for(int i=0; i < context->audioFrames; i++) {
    p->hw_outlets[0].buf[i] = *in++;
    p->hw_outlets[1].buf[i] = *in++;
  }

  process_patch(p);

  for(int i=0; i < context->audioFrames; i++) {
    for (unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
      audioWrite(context, i, channel, p->hw_inlets[channel].buf[i]);
    }
  }
}

void cleanup(BelaContext *context, void *userData)
{
  //TODO cleanup the patch, shutdown osc server
}

void interrupt_handler(int)
{
  gShouldStop = 1;
}

int main()
{
  // Set default settings
  BelaInitSettings settings;	// Standard audio settings
  Bela_defaultSettings(&settings);
        // you must have defined these function pointers somewhere and assign them to `settings` here.
        // only `settings.render` is required.
  settings.setup = &setup;
  settings.render = &render;
  settings.cleanup = &cleanup;

  // Initialise the PRU audio device
  if(Bela_initAudio(&settings, 0) != 0) {
                fprintf(stderr, "Error: unable to initialise audio");
  	return -1;
  }

  // Start the audio device running
  if(Bela_startAudio()) {
  	fprintf(stderr, "Error: unable to start real-time audio");
  	// Stop the audio device
  	Bela_stopAudio();
  	// Clean up any resources allocated for audio
  	Bela_cleanupAudio();
  	return -1;
  }

  // Set up interrupt handler to catch Control-C and SIGTERM
  signal(SIGINT, interrupt_handler);
  signal(SIGTERM, interrupt_handler);

  // Run until told to stop
  while(!gShouldStop) {
  	usleep(100000);
  }

  // Stop the audio device
  Bela_stopAudio();

  // Clean up any resources allocated for audio
  Bela_cleanupAudio();

  // All done!
  return 0;
}
