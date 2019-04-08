#include <Bela.h>
#include <cmath>
#include <csignal>
#include "protopatch.h"

patch *p;

bool setup(BelaContext *context, void *userData)
{
  p = new_patch();
  p->audio_opts.buf_size = context->audioFrames;
  p->audio_opts.sample_rate = context->audioSampleRate;
  p->audio_opts.hw_in_channels = context->audioInChannels;
  p->audio_opts.hw_out_channels = context->audioOutChannels;
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
  pp_connect(p, delay_l->id, 0, dac->id, 1);
  sort_patch(p);
	return true;
}

void render(BelaContext *context, void *userData)
{
  const float *in = (float*)context->audioIn;

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

  /* old
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		float out = 0.8 * sinf(gPhase);
		gPhase += 2.0 * M_PI * gFrequency * gInverseSampleRate;
		if(gPhase > 2.0 * M_PI)
			gPhase -= 2.0 * M_PI;

		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			// Two equivalent ways to write this code

			// The long way, using the buffers directly:
			// context->audioOut[n * context->audioOutChannels + channel] = out;

			// Or using the macros:
			audioWrite(context, n, channel, out);
		}
	}
  */
}

void cleanup(BelaContext *context, void *userData)
{

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
