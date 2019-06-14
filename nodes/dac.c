#include "baliset.h"

node *new_dac(const patch *p) {
  int num_inlets = p->audio_opts.hw_out_channels;
  node *n = init_node(p, num_inlets, 0, 0);
  n->process = &no_op;
  n->destroy = &no_op;
  for(int i = 0; i < num_inlets; i++){
    inlet *in = &n->inlets[i];
    in->buf = p->hw_inlets[i].buf;
    in->buf_size = p->hw_inlets[i].buf_size;
    in->num_connections = 0;
    in->connections = NULL;
  }
  return n;
}
