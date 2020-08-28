#include "baliset_graph.h"
#include <math.h>


//one pole highpass, ripped off from pd source code https://github.com/pure-data/pure-data/blob/master/src/d_filter.c
//out = normal * (new - last)
//y = (0.5 * (1 + coef)) * ((x[n] + coef * y[n-1]) - y[n-1])

//coefficient hertz to raw
typedef struct {
  unsigned int sample_rate;
  float last;
} hip_data;

hip_data *new_hip_data(const blst_patch *p) {
  hip_data *data = malloc(sizeof(hip_data));
  data->sample_rate = p->audio_opts.sample_rate;
  data->last = 0.0;
  return data;
}

void set_cutoff(blst_control *ctl, float f) {
  if(f < 0) f = 0;
  ctl->val = 1.0 - f * (2.0 * 3.14159) / 44100;
  if (ctl->val < 0)
    ctl->val = 0;
  else if (ctl->val > 1)
    ctl->val = 1;
}

void process_hip(blst_node *self) {
  hip_data *data = self->data;

  blst_outlet o_out = self->outlets[0];
  float *in_buf = self->inlets[0].buf;
  float *out_buf = o_out.buf;
  float coef = self->controls[0].val;
  float last = data->last;

  float out = 0.0;
  for(int i = 0; i < o_out.buf_size; i++) {
    float x = in_buf[i];
    out = (0.5 * (1 + coef)) * ((x + coef * last) - last);
    out_buf[i] = out;
    last = out;
  }
  data->last = last;
}

blst_node *blst_new_hip(const blst_patch *p) {
  blst_node *n = blst_init_node(p, 1, 1, 1);
  n->data = new_hip_data(p);
  n->process = &process_hip;
  n->controls[0].set_control = &set_cutoff;
  set_cutoff(&n->controls[0], 1.0);
  return n;
}
