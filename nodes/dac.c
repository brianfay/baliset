#include "baliset.h"
#include "math.h"

typedef struct {
  inlet *hw_inlets;
  /* float amp; */
  uint num_fade_samples;
} dac_data;

void set_amp(control *ctl, float val) {
  val = pow(10.0, ((100.0 * val) / 20.0 ) - 5.0);
  if(val >= 1.0) val = 1.0;
  if(val <= 1E-5) val = 0.0;
  ctl->val = val;
}

void process_dac(struct node *self) {
  dac_data *data = self->data;
  float amp = self->controls[0].val;
  //TODO this is expensive, can we somehow set at control-rate?
  /* amp = pow(10.0, ((100.0 * amp) / 20.0 ) - 5.0); */
  /* if(amp >= 1.0) amp = 1.0; */
  /* if(amp <= 0.0) amp = 0.0; */
  for(int i = 0; i < self->num_inlets; i++) {
    inlet *hw_out = &(data->hw_inlets[i]);
    float *in_buf = self->inlets[i].buf;
    for(int j = 0; j < hw_out->buf_size; j++) {
      //write result to hw output buffer
      hw_out->buf[j] += in_buf[j] * amp;
    }
  }
}

void destroy_dac(struct node *self) {
  destroy_inlets(self);
  free(self->controls);
}

node *new_dac(const patch *p) {
  int num_inlets = p->audio_opts.hw_out_channels;
  node *n = init_node(p, num_inlets, 0, 1);
  dac_data *data = malloc(sizeof(dac_data));
  data->hw_inlets = p->hw_inlets;
  /* data->amp = 1.0; */
  n->data = data;
  n->process = &process_dac;
  n->destroy = &destroy_dac;
  n->controls[0].val = 1.0; //volume
  n->controls[0].set_control = &set_amp;
  for(int i = 0; i < num_inlets; i++){
    init_inlet(p, n, i);
  }
  return n;
}

//when volume control changed
//fade to new target amp over 10 ms period
//if control changes again, start over the envelope

//0 - 100
//(10^(100*db/20 - 5))
