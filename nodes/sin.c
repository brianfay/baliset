#include "protopatch.h"
#include <math.h>

typedef struct {
  float phase;
  float phase_incr;
} sin_data;

void process_sin(struct node *self) {
  float *out_buf = self->outlets[0].buf;
  sin_data *data = self->data;
  inlet i_freq = self->inlets[0];
  inlet i_amp = self->inlets[1];
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    float freq = (i_freq.num_connections > 0) ? i_freq.buf[i] : i_freq.val;
    float amp = (i_amp.num_connections > 0) ? i_amp.buf[i] : i_amp.val;
    out_buf[i] = amp * sinf(data->phase);
    data->phase += freq * data->phase_incr;
    if(data->phase > M_PI * 2.0) {
      data->phase -= M_PI * 2.0;
    }
    if(data->phase < 0.0) {
      data->phase += M_PI * 2.0;
    }
  }
}

void destroy_sin(struct node *self) {
  destroy_inlets(self);
  destroy_outlets(self);
  free((sin_data*)self->data);
}

sin_data *new_sin_data(const patch *p){
  sin_data *d = malloc(sizeof(sin_data));
  d->phase = 0.0;
  d->phase_incr = (2.0 * M_PI / p->audio_opts.sample_rate);
  return d;
}

node *new_sin_osc(const patch *p) {
  node *n = new_node(p, 2, 1);
  n->data = new_sin_data(p);
  n->process = &process_sin;
  n->destroy = &destroy_sin;
  init_inlet(n, 0, "freq", 440.0);
  init_inlet(n, 1, "amp", 0.3);
  init_outlet(n, 0, "out");
  return n;
}
