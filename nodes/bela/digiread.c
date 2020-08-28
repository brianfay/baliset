#include "baliset.h"

typedef struct {
  blst_outlet *digital_outlets;
} digiread_data;

void process_digiread(blst_node *self) {
  digiread_data *d = self->data;
  for(int i = 0; i < self->num_outlets; i++) {
    blst_outlet *digi_out = &d->digital_outlets[i];
    float *out_buf = self->outlets[i].buf;
    for(int j = 0; j < digi_out->buf_size; j++) {
      out_buf[j] = digi_out->buf[j];
    }
  }
}

node *blst_new_digiread(const blst_patch *p){
  int num_outlets = p->audio_opts.digital_channels;
  blst_node *n = blst_init_node(p, 0, num_outlets, 0);
  n->process = &process_digiread;
  n->digital_outlets = p->digital_outlets;
  return n;
}
