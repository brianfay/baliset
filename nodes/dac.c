#include "baliset.h"

node *new_dac(const patch *p) {
  node *n = new_node(p, 0, 0);
  n->process = &no_op;
  n->destroy = &no_op;
  n->num_inlets = p->audio_opts.hw_out_channels;
  n->inlets = p->hw_inlets;
  return n;
}
