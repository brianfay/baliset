#include "baliset.h"
#include <math.h>

//inspired by Miller Puckette's wriitng on envelope followers
//for the envelope follower, take signal, square it, and lowpass filter
//Power of a window of samples is the average of each signal squared
//and the lowpass filter functions as moving average
// according to puckette, this "normalized one-pole lowpass" can be used:
// y[n] = p * y[n-1] + (1 - p)x[n]
//no idea what p should be, set it somewhere between 0 and 1

typedef struct {
  float *prev_samps; // stores one sample per outlet
  outlet *hw_outlets;
  uint num_attack_samples; //total number of samples for attack envelope
  uint *attack_countdown; //num samples until attack envelope is complete
  uint num_release_samples;
  uint *release_countdown;
  int gate_open;
} gate_adc_data;

gate_adc_data *new_gate_adc_data(const patch *p) {
  int num_outlets = p->audio_opts.hw_in_channels;
  gate_adc_data *gd = malloc(sizeof(gate_adc_data));
  gd->hw_outlets = p->hw_outlets;
  gd->prev_samps = calloc(num_outlets, sizeof(float));
  gd->num_attack_samples = (uint) ceil(p->audio_opts.sample_rate * 0.01); //10 ms?
  gd->num_release_samples = (uint) ceil(p->audio_opts.sample_rate * 0.5); //500 ms
  gd->attack_countdown = calloc(num_outlets, sizeof(uint));
  gd->release_countdown = calloc(num_outlets, sizeof(uint));
  gd->gate_open = 1;
  return gd;
}

void process_gate_adc(struct node *self) {
  gate_adc_data *data = self->data;
  float p = self->controls[0].val;
  float thresh = self->controls[1].val;
  for(int i = 0; i < self->num_outlets; i++){
    float prev_sample = data->prev_samps[i];
    outlet o_out = self->outlets[i];
    outlet *hw_in = &(data->hw_outlets[i]);
    float *out_buf = o_out.buf;
    float power = 0.0;
    for(int j = 0; j < o_out.buf_size; j++) {
      float sample = hw_in->buf[j];
      float amp = data->gate_open ? 1.0 : 0.0;
      uint attack_count = data->attack_countdown[i];
      uint release_count = data->release_countdown[i];
      if(attack_count) {
        //64 -> 0.0, 63 -> 1/64, 0 -> 64/64
        amp = (float)(data->num_attack_samples - attack_count) / (float)data->num_attack_samples;
        /* printf("num attack samps: %d\n", attack_count); */
        /* printf("attack amp: %9.6f\n", amp); */
        data->attack_countdown[i]--;
      }
      else if(release_count) {
        //64 -> 1.0, 0 -> 0.0, 1 -> 1/64
        amp = (float)release_count / (float)data->num_release_samples;
        /* printf("num release samps: %d\n", release_count); */
        /* printf("release amp: %9.6f\n", amp); */
        data->release_countdown[i]--;
      }

      /* out_buf[j] = sample; */
      out_buf[j] = amp * sample;
      sample *= sample;
      power = p * prev_sample + (1.0 - p) * sample;
      //could use sqrt to get RMS value, not sure if that makes a difference
      prev_sample = power;

      if (attack_count == 0 && release_count == 0) {
        /* printf("thresh: %9.6f\n", thresh); */
        /* printf("power: %9.6f\n", power); */
        if (data->gate_open && power <= thresh) {
          data->gate_open = 0;
          data->release_countdown[i] = data->num_release_samples;
        }
        else if (!data->gate_open && power > thresh) {
          data->gate_open = 1;
          data->attack_countdown[i] = data->num_attack_samples;
        }
      }
    }
    data->prev_samps[i] = power;
  }
}

void destroy_gate_adc(struct node *self) {
  destroy_outlets(self);
}

node *new_gate_adc(const patch *p) {
  int num_outlets = p->audio_opts.hw_in_channels;
  node *n = init_node(p, 0, num_outlets, 1);
  n->data = new_gate_adc_data(p);
  n->process = &process_gate_adc;
  n->destroy = &destroy_gate_adc;
  n->controls[0].val = 0.5; //filter coefficient
  n->controls[1].val = 0.1; //threshold coefficient
  for(int i = 0; i < num_outlets; i++) {
    init_outlet(p, n, i);
  }
  return n;
}
