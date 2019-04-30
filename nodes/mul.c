#include "baliset.h"

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
  node *n = init_node(p, 2, 1);
  n->process = &process_mul;
  n->destroy = &destroy_mul;
  init_inlet(n, 0, "a", 0.0);
  init_inlet(n, 1, "b", 0.0);
  init_outlet(n, 0, "out");
  return n;
}
