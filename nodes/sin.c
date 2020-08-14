#include "baliset_graph.h"
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
    float freq = (i_freq.num_connections > 0) ? i_freq.buf[i] : 220.;
    float amp = (i_amp.num_connections > 0) ? i_amp.buf[i] : 0.3;
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
  node *n = init_node(p, 2, 1, 0);
  n->data = new_sin_data(p);
  n->process = &process_sin;
  n->destroy = &destroy_sin;
  //freq
  init_inlet(p, n, 0);
  //amp
  init_inlet(p, n, 1);
  //out
  init_outlet(p, n, 0);
  return n;
}
