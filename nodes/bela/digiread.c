#include "baliset.h"

node *new_digiread(const patch *p){
  node *n = new_node(p, 0, 0);
  n->process = &no_op;
  n->destroy = &no_op;
  n->num_outlets = p->audio_opts.digital_channels;
  n->outlets = p->digital_outlets;
  return n;
}
