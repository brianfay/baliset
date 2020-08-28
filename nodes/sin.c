#include "baliset_graph.h"
#include "math.h"
#include <stdint.h>


//using fixed point math for the phasor, referenced https://pbat.ch/wiki/osc/
typedef struct {
  int32_t lphs;
  int32_t maxlen;
  int32_t pmask;
  int32_t lbmask;
  int nlb;
  int inc;
  float inlb;
  float maxlens;
  const float *wt;
} sin_data;

void process_sin(blst_node *self) {
  float *out_buf = self->outlets[0].buf;
  sin_data *d = self->data;
  blst_inlet i_freq = self->inlets[0];
  blst_inlet i_amp = self->inlets[1];
  int32_t phs = d->lphs;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    float freq = (i_freq.num_connections > 0) ? i_freq.buf[i] : 220.;
    float amp = (i_amp.num_connections > 0) ? i_amp.buf[i] : 0.3;
    d->inc = (int)round(freq * d->maxlens);
    int pos = phs >> d->nlb;
    float x1 = d->wt[pos];
    float x2 = d->wt[(pos + 1) % WAVETABLE_SIZE];
    float frac = (phs & d->lbmask) * d->inlb;
    out_buf[i] = amp * (x1 * (1.0 - frac) + x2 * frac);
    phs = phs + (int32_t)d->inc;
    phs &= d->pmask;
  }
  d->lphs = phs;
}

sin_data *new_sin_data(const blst_patch *p){
  sin_data *d = malloc(sizeof(sin_data));
  d->maxlen = 0x1000000L; //using 28 bits to store phasor
  d->pmask  = 0x0FFFFFFL; //& with this mask to get back in range
  d->nlb = 0;
  int32_t tmp;
  tmp = d->maxlen / WAVETABLE_SIZE;
  while (tmp >>=1) d->nlb++;
  /* d->nlb = 18; */
  d->maxlens = (float)d->maxlen / (float)p->audio_opts.sample_rate;
  d->inlb = 1.0 / (1<<d->nlb);
  d->lbmask = (1 << d->nlb) - 1; //& with this mask to get the lower bits (storing the fractional portion)
  d->lphs = 0;
  d->inc = 0;

  d->wt = p->wavetables.sine_table;
  return d;
}

blst_node *blst_new_sin_osc(const blst_patch *p) {
  blst_node *n = blst_init_node(p, 2, 1, 0);
  n->data = new_sin_data(p);
  n->process = &process_sin;
  return n;
}
