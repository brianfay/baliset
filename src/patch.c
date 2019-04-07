#include "protopatch.h"
#include <io.h>

static int audioCallback (const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {

  float *in = (float*)inputBuffer;

  patch *p = (patch*) userData;

  for(int i=0; i < framesPerBuffer; i++) {
    p->hw_outlets[0].buf[i] = *in++;
    p->hw_outlets[1].buf[i] = *in++;
  }

  process_patch(p);

  float *out = (float*)outputBuffer;

  for(int i=0; i < framesPerBuffer; i++) {
    *out++ = p->hw_inlets[0].buf[i];
    *out++ = p->hw_inlets[1].buf[i];
  }
  return 0;
}

int main() {
  srand(0);
  /* FM-y example
  patch *p = new_patch();
  node *sin = new_sin_osc(p);
  node *lfo = new_sin_osc(p);
  node *lfo2 = new_sin_osc(p);
  set_control(lfo, "amp", 550.0);
  set_control(lfo2, "freq", 0.03);
  set_control(lfo2, "amp", 30.0);
  node *dac = new_dac(p);
  add_node(p, dac);
  add_node(p, sin);
  add_node(p, lfo);
  add_node(p, lfo2);
  //add_node(g, sin2);
  pp_connect(p, lfo->id, 0, sin->id, 0);
  pp_connect(p, lfo2->id, 0, lfo->id, 0);
  pp_connect(p, sin->id, 0, dac->id, 0);
  pp_connect(p, sin->id, 0, dac->id, 1);

  sort_patch(p);
  pa_run(audioCallback, p);
  */

  patch *p = new_patch();
  node *adc = new_adc(p);
  node *dac = new_dac(p);
  add_node(p, adc);
  add_node(p, dac);
  pp_connect(p, adc->id, 0, dac->id, 0);
  pp_connect(p, adc->id, 1, dac->id, 1);
  sort_patch(p);
  pa_run(audioCallback, p);

  while(1){
    Pa_Sleep(3000);
  }

  free_patch(p);
}
