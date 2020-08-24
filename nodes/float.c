#include "baliset_graph.h"

void process_float(blst_node *self) {
  float *out_buf = self->outlets[0].buf;
  float val = self->controls[0].val;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    out_buf[i] = val;
  }
}

blst_node *blst_new_float(const blst_patch *p) {
  blst_node *n = blst_init_node(p, 0, 1, 1);
  blst_init_outlet(p, n, 0);
  n->process = &process_float;
  n->controls[0].val = 0.0;
  return n;
}
