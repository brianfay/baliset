#include "baliset_graph.h"
#include "soundpipe.h"

typedef struct {
  sp_data *sp;
  sp_buthp *buthp;
} buthp_data;


buthp_data *new_buthp_data(const blst_patch *p) {
  buthp_data *data = malloc(sizeof(buthp_data));
  sp_create(&data->sp);
  sp_buthp_create(&data->buthp);
  sp_buthp_init(data->sp, data->buthp);
  return data;
}

void process_buthp(blst_node *self) {
  blst_outlet o_out = self->outlets[0];
  float *out_buf = self->outlets[0].buf;
  float *in_buf = self->inlets[0].buf;

  buthp_data *data = self->data;
  sp_data *sp = data->sp;
  sp_buthp *buthp = data->buthp;

  buthp->freq = self->controls[0].val;

  for(int i = 0; i < o_out.buf_size; i++) {
    sp_buthp_compute(sp, buthp, &in_buf[i], &out_buf[i]);
  }
}

void destroy_buthp(blst_node *self) {
  buthp_data *data = self->data;
  sp_buthp_destroy(&data->buthp);
  sp_destroy(&data->sp);
}

blst_node *blst_new_buthp(const blst_patch *p) {
  blst_node *n = blst_init_node(p, 1, 1, 1);
  buthp_data *data = new_buthp_data(p);

  n->data = data;
  n->process = &process_buthp;
  n->destroy = &destroy_buthp;

  n->controls[0].val = 5.0;

  blst_init_inlet(p, n, 0);
  blst_init_outlet(p, n, 0);

  return n;
}
