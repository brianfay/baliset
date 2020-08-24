#include "baliset_graph.h"

void process_mul(blst_node *self) {
  float *out_buf = self->outlets[0].buf;
  float *in_buf_a = self->inlets[0].buf;
  float *in_buf_b = self->inlets[1].buf;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    out_buf[i] = in_buf_a[i] * in_buf_b[i];
  }
}

blst_node *blst_new_mul(const blst_patch *p) {
  blst_node *n = blst_init_node(p, 2, 1, 0);
  n->process = &process_mul;
  //a
  blst_init_inlet(p, n, 0);
  //b
  blst_init_inlet(p, n, 1);
  //out
  blst_init_outlet(p, n, 0);
  return n;
}
