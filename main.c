#include "protopatch.h"
#include <io.h>

static int audioCallback (const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {

  graph *g = (graph*) userData;

  struct int_stack s = sort_graph(g);

  process_graph(g, s);

  float *out = (float*)outputBuffer;
  node *n = get_node(g, 1);
  float *buf = n->outlets[0].buf;

  for(int i=0; i < framesPerBuffer; i++) {
    //*out++ = buf[i];
    //*out++ = buf[i];
    *out++ = g->hw_inlets[0].buf[i];
    *out++ = g->hw_inlets[1].buf[i];
  }
  return 0;
}

int main() {
  srand(0);
  graph *g = new_graph();
  node *sin = new_sin_osc(g);
  node *lfo = new_sin_osc(g);
  node *lfo2 = new_sin_osc(g);
  sin_data *lfo_data = lfo->data;
  sin_data *lfo2_data = lfo2->data;
  lfo_data->freq = 0.5;
  lfo_data->amp = 550.0;
  lfo2_data->freq = 0.03;
  lfo2_data->amp = 30.0;
  node *dac = new_dac(g);
  add_node(g, dac);
  add_node(g, sin);
  add_node(g, lfo);
  add_node(g, lfo2);
  //add_node(g, sin2);
  pp_connect(g, lfo->id, 0, sin->id, 0);
  pp_connect(g, lfo2->id, 0, lfo->id, 0);
  pp_connect(g, sin->id, 0, dac->id, 0);
  pp_connect(g, sin->id, 0, dac->id, 1);

  pa_run(audioCallback, g);

  while(1){
    Pa_Sleep(3000);
    //node *n = new_sin_osc(g);
    //int freq = (1 + (rand() % 9)) * 110.0;
    //((sin_data*) n->data)->freq = freq;
    //add_node(g, n);
    //pp_connect(g, n->id, 0, dac->id, rand() % 2);
  }

  free_graph(g);
}
