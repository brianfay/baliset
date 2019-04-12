#include "protopatch.h"

node *new_adc(const patch *p) {
  node *n = malloc(sizeof(node));
  n->process = &no_op;
  n->destroy = &no_op;
  n->num_inlets = 0;
  n->num_outlets = p->audio_opts.hw_out_channels;
  n->outlets = p->hw_outlets;
  n->num_controls = 0;
  n->last_visited = -1;
  return n;
}
