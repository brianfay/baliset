#include "protopatch.h"

node *new_adc(const patch *p) {
  node *n = new_node(p, 0, 0);
  n->process = &no_op;
  n->destroy = &no_op;
  n->num_outlets = p->audio_opts.hw_out_channels;
  n->outlets = p->hw_outlets;
  return n;
}
