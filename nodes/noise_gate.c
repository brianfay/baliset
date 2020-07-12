#include "baliset.h"
#include <math.h>

typedef struct {
  uint num_attack_samples; //total number of samples for attack envelope
  uint num_release_samples;
  uint attack_countdown;
  uint release_countdown;
  uint num_rms_samples;
  uint rms_count;
  float rms_accum;
  int gate_open;
  uint num_gate_close_samples;
  uint gate_close_countdown;
  float rms_val;
} noise_gate_data;

noise_gate_data *new_noise_gate_data(const patch *p) {
  noise_gate_data *nd = malloc(sizeof(noise_gate_data));
  nd->num_attack_samples = (uint) ceil(p->audio_opts.sample_rate * 0.001);
  printf("setting num_attack_samples to %d\n", nd->num_attack_samples);
  nd->num_release_samples = (uint) ceil(p->audio_opts.sample_rate * 0.01);
  printf("setting num_release_samples to %d\n", nd->num_release_samples);
  nd->attack_countdown = 0;
  nd->release_countdown = 0;
  nd->num_rms_samples = (uint) ceil(p->audio_opts.sample_rate * 0.01); //0.05 50 ms is effective but latency sux
  nd->num_gate_close_samples = (uint) ceil(p->audio_opts.sample_rate * 0.5);
  nd->gate_close_countdown = 0;
  nd->rms_count = 0;
  nd->rms_accum = 0.0;
  nd->rms_val = 0.0;
  nd->gate_open = 1;
  return nd;
}
void noise_gate_set_threshold(control *ctl, float val) {
  //convert db to amp, expect a range from like -100db to 0db (0 meaning unity gain)
  val = pow(10.0, val / 20.0);
  if(val >= 1.0) val = 1.0;
  if(val <= 0.0) val = 0.0;
  ctl->val = val;
}

void process_noise_gate(struct node *self) {
  noise_gate_data *data = self->data;
  outlet o_out = self->outlets[0];
  float *out_buf = o_out.buf;
  inlet i_in = self->inlets[0];
  float *in_buf = i_in.buf;
  float thresh = self->controls[0].val;

  float amp = data->gate_open ? 1.0 : 0.0;

  if(!data->gate_open && data->gate_close_countdown > 0) {
    data->gate_close_countdown--;
  }

  for(int i = 0; i < o_out.buf_size; i++) {
    float x = in_buf[i];

    if (data->attack_countdown) {
      amp = (float)(data->num_attack_samples - data->attack_countdown) / (float)data->num_attack_samples;
      /* printf("amp: %9.6f, attack countdown: %d\n", amp, data->attack_countdown); */
      data->attack_countdown--;
    } else if (data->release_countdown) {
      amp = (float) data->release_countdown / (float)data->num_release_samples;
      /* printf("amp: %9.6f, release countdown: %d\n", amp, data->release_countdown); */
      data->release_countdown--;
    }

    data->rms_count = (data->rms_count + 1) % data->num_rms_samples;
    if(data->rms_count == 0) {
      data->rms_val = sqrtf((float)data->rms_accum / (float)data->num_rms_samples);
      data->rms_val = (float)data->rms_accum / (float)data->num_rms_samples;
      data->rms_accum = 0.0;
    } else {
      data->rms_accum += (x * x);
    }

    if (data->gate_open && data->rms_val <= thresh) {
      printf("closing gate, rms_val: %9.6f, thresh: %9.6f\n", data->rms_val, thresh);
      data->gate_open = 0;
      data->release_countdown = data->num_release_samples;
    } else if (!data->gate_open && data->rms_val > thresh) {
      printf("opening gate, rms_val: %9.6f, thresh: %9.6f\n", data->rms_val, thresh);
      data->gate_open = 1;
      data->attack_countdown = data->num_attack_samples;
      data->gate_close_countdown = data->num_gate_close_samples;
    }
    out_buf[i] = amp * x;
  }

}

void destroy_noise_gate(struct node *self) {
  destroy_inlets(self);
  destroy_outlets(self);
  free(self->controls);
}

node *new_noise_gate(const patch *p) {
  node *n = init_node(p, 1, 1, 1);
  n->process = &process_noise_gate;
  n->destroy = &destroy_noise_gate;

  n->data = new_noise_gate_data(p);
  n->controls[0].val = 1E-5;
  n->controls[0].set_control = &noise_gate_set_threshold;

  init_inlet(p, n, 0);
  init_outlet(p, n, 0);

  return n;
}
