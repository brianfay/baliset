#include "protopatch.h"
#include <math.h>
/*
typedef struct {
  float amp;
  float phase;
  float freq;
  float phase_incr;
} sin_data;
*/

void process_sin (struct node *self) {
  float *buf = self->outlets[0].buf;
  sin_data *data = self->data;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    inlet freq = self->inlets[0];
    if(freq.num_connections > 0) {
      data->freq = freq.buf[i];
    }
    buf[i] = data->amp * sinf(data->phase);
    data->phase += data->freq * data->phase_incr;
    if(data->phase > M_PI * 2.0) {
      data->phase -= M_PI * 2.0;
    }
    if(data->phase < 0.0) {
      data->phase += M_PI * 2.0;
    }
  }
}

void destroy_sin (struct node *self) {
  destroy_inlets(self);
  destroy_outlets(self);
  free((sin_data*)self->data);
}

sin_data *new_sin_data(patch *p){
  sin_data *d = malloc(sizeof(sin_data));
  d->amp = 0.3;
  d->freq = 440.0;
  d->phase = 0.0;
  d->phase_incr = (2.0 * M_PI / p->audio_opts.sample_rate);
  return d;
}

node *new_sin_osc(patch *p) {
  node *n = malloc(sizeof(node));
  n->data = new_sin_data(p);
  n->process = &process_sin;
  n->destroy = &destroy_sin;
  n->num_inlets = 1;//TODO create_inlet convenience function
  n->num_outlets = 1;
  n->num_controls = 0;
  n->last_visited = -1;
  n->inlets = malloc(sizeof(inlet) * n->num_inlets);
  n->inlets[0] = new_inlet(64, "freq");
  n->outlets = malloc(sizeof(outlet));
  n->outlets[0] = new_outlet(64, "out");
  return n;
}
