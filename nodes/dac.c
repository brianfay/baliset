#include "baliset_graph.h"
#include "math.h"

typedef struct {
  blst_inlet *hw_inlets;
  /* float amp; */
  /* uint num_fade_samples; */
} dac_data;

//TODO just use plain old db
static void set_amp(blst_control *ctl, float val) {
  val = pow(10.0, ((100.0 * val) / 20.0 ) - 5.0);
  if(val >= 1.0) val = 1.0;
  if(val <= 1E-5) val = 0.0;
  ctl->val = val;
}

void process_dac(blst_node *self) {
  dac_data *data = self->data;
  float amp = self->controls[0].val;
  for(int i = 0; i < self->num_inlets; i++) {
    blst_inlet *hw_out = &(data->hw_inlets[i]);
    float *in_buf = self->inlets[i].buf;
    for(int j = 0; j < hw_out->buf_size; j++) {
      //write result to hw output buffer
      hw_out->buf[j] += in_buf[j] * amp;
    }
  }
}

blst_node *blst_new_dac(const blst_patch *p) {
  int num_inlets = p->audio_opts.hw_out_channels;
  blst_node *n = blst_init_node(p, num_inlets, 0, 1);
  dac_data *data = malloc(sizeof(dac_data));
  data->hw_inlets = p->hw_inlets;
  /* data->amp = 1.0; */
  n->data = data;
  n->process = &process_dac;
  n->controls[0].val = 1.0; //volume
  n->controls[0].set_control = &set_amp;
  return n;
}
