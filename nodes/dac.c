#include "protopatch.h"

void no_op (struct node *self) {
  return;
}

node *new_dac(graph *g) {
  node *n = malloc(sizeof(node));
  n->process = &no_op;
  n->destroy = &no_op;
  n->num_inlets = g->num_hw_inlets;
  n->num_outlets = 0;
  n->num_controls = 0;
  n->last_visited = -1;
  n->inlets = g->hw_inlets;
  return n;
}



