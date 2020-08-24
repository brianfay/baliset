#include "baliset.h"

//TODO don't directly share hw outlets, will break nice memory cleanup
node *blst_new_digiread(const blst_patch *p){
  int num_outlets = p->audio_opts.digital_channels;
  blst_node *n = blst_init_node(p, 0, num_outlets, 0);
  for(int i = 0; i < num_outlets; i++) {
    blst_outlet *out = &n->outlets[i];
    out->buf = p->digital_outlets[i].buf;
    out->buf_size = p->digital_outlets[i].buf_size;
    out->num_connections = 0;
    out->connections = NULL;
  }
  return n;
}
