#include "baliset_graph.h"
#include "math.h"

typedef struct {
  blst_outlet *hw_outlets;
} adc_data;

static void set_amp(blst_control *ctl, float val) {
  val = pow(10.0, ((100.0 * val) / 20.0 ) - 5.0);
  if(val >= 1.0) val = 1.0;
  if(val <= 1E-5) val = 0.0;
  ctl->val = val;
}

void process_adc(blst_node *self) {
  adc_data *data = self->data;
  float amp = self->controls[0].val;
  for(int i = 0; i < self->num_outlets; i++) {
    blst_outlet *hw_out = &(data->hw_outlets[i]);
    float *out_buf = self->outlets[i].buf;
    for(int j = 0; j < hw_out->buf_size; j++) {
      out_buf[j] = hw_out->buf[j] * amp;
    }
  }
}

blst_node *blst_new_adc(const blst_patch *p) {
  int num_outlets = p->audio_opts.hw_in_channels;
  blst_node *n = blst_init_node(p, 0, num_outlets, 1);
  n->process = &process_adc;
  n->controls[0].val = 1.0;
  n->controls[0].set_control = &set_amp;
  adc_data *data = malloc(sizeof(adc_data));
  data->hw_outlets = p->hw_outlets;
  n->data = data;
  return n;
}
