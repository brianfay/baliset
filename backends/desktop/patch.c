#include "baliset.h"
#include "string.h"
#include <io.h>
#include "tinypipe.h"

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
  tpipe_init(&rt_consumer_pipe, 1024 * 10); //might want to make some kind of baliset_init for this
  /* tpipe_clear(&rt_consumer_pipe); */
  /* FM-y example*/
  audio_options a = {.buf_size = 64, .sample_rate = 44100,
                     .hw_in_channels = 2, .hw_out_channels = 2};
  patch *p = new_patch(a);
  node *sin = new_sin_osc(p);
  /* node *lfo = new_sin_osc(p); */
  /* node *lfo2 = new_sin_osc(p); */
  node *dac = new_dac(p);
  /* set_control(lfo, "amp", 550.0); */
  /* set_control(lfo2, "freq", 0.03); */
  /* set_control(lfo2, "amp", 30.0); */
  add_node(p, sin);
  /* add_node(p, lfo); */
  /* add_node(p, lfo2); */
  add_node(p, dac);
  /* blst_connect(p, lfo->id, 0, sin->id, 0); */
  /* blst_connect(p, lfo2->id, 0, lfo->id, 0); */
  /* blst_connect(p, sin->id, 0, dac->id, 0); */
  /* blst_connect(p, sin->id, 0, dac->id, 1); */

  sort_patch(p);
  pa_run(audioCallback, p);
  

  /*
  audio_options a = {.buf_size = 64, .sample_rate = 44100,
                     .hw_in_channels = 2, .hw_out_channels = 2};
  patch *p = new_patch(a);
  node *adc = new_adc(p);
  node *delay_l = new_delay(p);
  node *delay_r = new_delay(p);
  node *dac = new_dac(p);
  node *loop = new_looper(p);
  add_node(p, adc);
  add_node(p, delay_l);
  add_node(p, delay_r);
  add_node(p, loop);
  add_node(p, dac);

  printf("dac id: %d\n", dac->id);

  set_control(delay_l, "delay_time", 0.62);
  set_control(delay_r, "delay_time", 1.44);

  blst_connect(p, adc->id, 0, delay_l->id, 0);
  blst_connect(p, adc->id, 1, delay_r->id, 0);
  blst_connect(p, delay_l->id, 0, dac->id, 0);
  blst_connect(p, delay_r->id, 0, dac->id, 1);
  sort_patch(p);
  pa_run(audioCallback, p);
  */

  run_osc_server(p);

  /* while(1) {Pa_Sleep(3000);} */

  free_patch(p);
  tpipe_free(&rt_consumer_pipe);
}
