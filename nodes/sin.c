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
  node *n = malloc(sizeof(node));
  n->data = new_sin_data(p);
  n->last_visited = -1;
  n->process = &process_sin;
  n->destroy = &destroy_sin;
  n->num_inlets = 2;//TODO create_inlet convenience function
  n->num_outlets = 1;
  n->num_controls = 0;
  n->inlets = malloc(sizeof(inlet) * n->num_inlets);
  n->inlets[0] = new_inlet(p->audio_opts.buf_size, "freq", 440.0);
  n->inlets[1] = new_inlet(p->audio_opts.buf_size, "amp", 0.3);
  n->outlets = malloc(sizeof(outlet));
  n->outlets[0] = new_outlet(p->audio_opts.buf_size, "out");
  return n;
}
