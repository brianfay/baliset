#include "protopatch.h"

node *new_dac(const patch *p) {
  node *n = malloc(sizeof(node));
  n->process = &no_op;
  n->destroy = &no_op;
  n->num_inlets = p->audio_opts.hw_out_channels;
  n->num_outlets = 0;
  n->num_controls = 0;
  n->last_visited = -1;
  n->inlets = p->hw_inlets;
  return n;
}
