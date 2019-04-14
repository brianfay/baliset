#include "protopatch.h"

node *new_digiread(const patch *p){
  node *n = malloc(sizeof(node));
  n->last_visited = -1;
  n->process = &no_op;
  n->destroy = &no_op;
  n->num_inlets = 0;
  n->num_outlets = p->audio_opts.digital_channels;
  n->outlets = p->digital_outlets;
  n->num_controls = 0;
  return n;
}
