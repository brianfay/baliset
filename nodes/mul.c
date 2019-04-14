#include "protopatch.h"

void process_mul(struct node *self) {
  float *out_buf = self->outlets[0].buf;
  float *in_buf_a = self->inlets[0].buf;
  float *in_buf_b = self->inlets[1].buf;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    out_buf[i] = in_buf_a[i] * in_buf_b[i];
  }
}

void destroy_mul(struct node *self) {
  destroy_inlets(self);
  destroy_outlets(self);
}

node *new_mul(const patch *p) {
  node *n = malloc(sizeof(node));
  n->last_visited = -1;
  n->process = &process_mul;
  n->destroy = &destroy_mul;
  n->num_inlets = 2;
  n->num_outlets = 1;
  n->num_controls = 0;
  n->inlets = malloc(sizeof(inlet) * n->num_inlets);
  n->inlets[0] = new_inlet(p->audio_opts.buf_size, "a", 0.0);
  n->inlets[1] = new_inlet(p->audio_opts.buf_size, "b", 0.0);
  n->outlets = malloc(sizeof(outlet));
  n->outlets[0] = new_outlet(p->audio_opts.buf_size, "out");
  return n;
}
