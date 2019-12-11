#include "baliset.h"

typedef struct {
  inlet *hw_inlets;
} dac_data;

void process_dac(struct node *self) {
  dac_data *data = self->data;
  float amp = self->controls[0].val;
  if(amp >= 1.0) amp = 1.0;
  if(amp <= 0.0) amp = 0.0;
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
  n->data = data;
  n->process = &process_dac;
  n->destroy = &destroy_dac;
  n->controls[0].val = 1.0; //volume
  for(int i = 0; i < num_inlets; i++){
    init_inlet(p, n, i);
  }
  return n;
}
