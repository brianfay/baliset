#include "baliset_graph.h"

void process_float(struct node *self) {
  float *out_buf = self->outlets[0].buf;
  float val = self->controls[0].val;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    out_buf[i] = val;
  }
}

node *new_float(const patch *p) {
  node *n = init_node(p, 0, 1, 1);
  init_outlet(p, n, 0);
  n->process = &process_float;
  n->controls[0].val = 0.0;
  return n;
}
