#include "baliset_graph.h"
#include "soundpipe.h"

typedef struct {
  sp_data *sp;
  sp_dist *ds;
} dist_data;

void process_dist(struct node *self) {
  outlet o_out = self->outlets[0];
  float *out_buf = self->outlets[0].buf;
  float *in_buf = self->inlets[0].buf;

  dist_data *data = self->data;
  sp_data *sp = data->sp;
  sp_dist *ds = data->ds;

  ds->pregain = self->controls[0].val;
  ds->postgain = self->controls[1].val;
  ds->shape1 = self->controls[2].val;
  ds->shape2 = self->controls[3].val;
  for(int i = 0; i < o_out.buf_size; i++) {
    sp_dist_compute(sp, ds, &in_buf[i], &out_buf[i]);
  }
}

void destroy_dist(struct node *self) {
  destroy_inlets(self);
  destroy_outlets(self);
  free(self->controls);
}

node *new_dist(const patch *p) {
  node *n = init_node(p, 1, 1, 4);
  dist_data *data = malloc(sizeof(dist_data));
  n->data = data;
  n->process = &process_dist;
  n->destroy = &destroy_dist;

  sp_create(&data->sp);
  sp_dist_create(&data->ds);

  //in
  init_inlet(p, n, 0);
  //out
  init_outlet(p, n, 0);

  //pregain
  n->controls[0].val = 2.0;
  //postgain
  n->controls[1].val = 0.5;
  //shape1
  n->controls[2].val = 0.0;
  //shape2
  n->controls[3].val = 0.0;

  return n;
}
