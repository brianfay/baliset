#include "baliset_graph.h"

node *new_adc(const patch *p) {
  int num_outlets = p->audio_opts.hw_in_channels;
  node *n = init_node(p, 0, num_outlets, 0);
  n->process = &no_op;
  n->destroy = &no_op;
  for(int i = 0; i < num_outlets; i++) {
    outlet *out = &n->outlets[i];
    out->buf = p->hw_outlets[i].buf;
    out->buf_size = p->hw_outlets[i].buf_size;
    out->num_connections = 0;
    out->connections = NULL;
  }
  return n;
}
