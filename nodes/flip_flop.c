#include "baliset_graph.h"

typedef struct {
  int flipped;
  float prev_trig_value;
} flip_flop_data;

flip_flop_data *new_flip_flop_data(const blst_patch *p) {
  flip_flop_data *d = malloc(sizeof(flip_flop_data));
  d->flipped = 0;
  d->prev_trig_value = 0.0;
  return d;
}

//TODO xfade on transition?
void process_flip_flop(blst_node *self) {
  flip_flop_data *d = self->data;
  blst_inlet i_in = self->inlets[0];
  blst_inlet i_trig = self->inlets[1];
  blst_outlet o_a = self->outlets[0];
  blst_outlet o_b = self->outlets[1];
  float *in_buf = i_in.buf;
  float *trig_buf = i_trig.buf;
  float ctl_trig = self->controls[0].val;
  for(int i=0; i < o_a.buf_size; i++) {
    float new_trig_samp = i_trig.num_connections > 0 ? trig_buf[i] : ctl_trig;
    if(d->prev_trig_value <= 0.0 && new_trig_samp > 0.0) {
      d->flipped = !d->flipped;
    }
    d->prev_trig_value = new_trig_samp;
    //loud outlet
    (d->flipped ? o_b.buf : o_a.buf)[i] = in_buf[i];
    //quiet outlet
    (d->flipped ? o_a.buf : o_b.buf)[i] = 0.0;
  }
}

blst_node *blst_new_flip_flop(const blst_patch *p) {
  blst_node *n = blst_init_node(p, 2, 2, 1);
  n->data = new_flip_flop_data(p);
  n->process = &process_flip_flop;

  //flip_it toggle
  n->controls[0].val = 0.0;

  blst_init_inlet(p, n, 0);

  //trig inlet
  blst_init_inlet(p, n, 1);

  blst_init_outlet(p, n, 0);
  blst_init_outlet(p, n, 1);

  return n;
}
