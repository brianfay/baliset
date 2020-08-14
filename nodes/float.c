#include "baliset_graph.h"

/* typedef struct { */
/*   float value; */
/* } float_data; */

void process_float(struct node *self) {
  float *out_buf = self->outlets[0].buf;
  /* float_data *data = self->data; */
  float val = self->controls[0].val;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    out_buf[i] = val;
  }
}

void destroy_float(struct node *self) {
  destroy_outlets(self);
  /* free((float_data*)self->data); */
}

/* float_data *new_float_data() { */
/*   float_data *data = malloc(sizeof(float_data)); */
/*   data->value = 0.0; */
/*   return data; */
/* } */

node *new_float(const patch *p) {
  node *n = init_node(p, 0, 1, 1);
  init_outlet(p, n, 0);
  /* n->data = new_float_data(p); */
  n->process = &process_float;
  n->destroy = &destroy_float;
  n->controls[0].val = 0.0;
  return n;
}
