#include "baliset_graph.h"

void process_mul(struct node *self) {
  float *out_buf = self->outlets[0].buf;
  float *in_buf_a = self->inlets[0].buf;
  float *in_buf_b = self->inlets[1].buf;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    out_buf[i] = in_buf_a[i] * in_buf_b[i];
  }
}

node *new_mul(const patch *p) {
  node *n = init_node(p, 2, 1, 0);
  n->process = &process_mul;
  //a
  init_inlet(p, n, 0);
  //b
  init_inlet(p, n, 1);
  //out
  init_outlet(p, n, 0);
  return n;
}
