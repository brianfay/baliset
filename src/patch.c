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
    //*out++ = buf[i];
    //*out++ = buf[i];
    *out++ = p->hw_inlets[0].buf[i];
    *out++ = p->hw_inlets[1].buf[i];
  }
  return 0;
}

int main() {
  srand(0);
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

  while(1){
    Pa_Sleep(3000);
    printf("%.6f\n", (rand() % 100 / 100.0));
    /* set_control(lfo, "freq", (rand() % 100 / 100.0)); */
    set_control(lfo2, "freq", (rand() % 100 / 100.0));
    //node *n = new_sin_osc(g);
    //int freq = (1 + (rand() % 9)) * 110.0;
    //((sin_data*) n->data)->freq = freq;
    //add_node(g, n);
    //pp_connect(g, n->id, 0, dac->id, rand() % 2);
  }

  free_patch(p);
}
