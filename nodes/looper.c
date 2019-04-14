#include "protopatch.h"

typedef struct {
  float *buf;
  unsigned int recording;
  unsigned int rw_head;
  unsigned int loop_buf_size;
  float prev_trig_samp;
  unsigned int loop_length;
} looper_data;

looper_data *new_looper_data(const patch *p) {
  looper_data *ld = malloc(sizeof(looper_data));
  ld->loop_buf_size = p->audio_opts.sample_rate * 180;
  ld->buf = calloc(ld->loop_buf_size, sizeof(float)); //3 minutes, really long looper
  ld->recording = 0;
  ld->rw_head = 0;
  ld->loop_length = 0;
  ld->prev_trig_samp = 0.0;
  return ld;
}

void process_looper(struct node *self) {
  looper_data *ld = self->data;
  inlet i_in = self->inlets[0];
  inlet i_trig = self->inlets[1];
  float *in_buf = i_in.buf;
  float *trig_buf = i_trig.buf;
  outlet o_out = self->outlets[0];
  float *out_buf = o_out.buf;
  for(int i=0; i < o_out.buf_size; i++) {
    float new_trig_samp = trig_buf[i];
    if(ld->prev_trig_samp <= 0.0 && new_trig_samp > 0.0) {
      if(ld->recording) {ld->loop_length = ld->rw_head;}
      ld->loop_length %= ld->loop_buf_size;
      ld->rw_head = 0;
      ld->recording = (ld->recording + 1) % 2; //alternate between 0 and 1
    }
    ld->prev_trig_samp = new_trig_samp;

    if(ld->recording) {
      out_buf[i] = 0.0;
      ld->buf[ld->rw_head] = in_buf[i];
      ld->rw_head = (ld->rw_head + 1) % ld->loop_buf_size;
    }else {
      out_buf[i] = ld->buf[ld->rw_head];
      ld->rw_head++;
      if(ld->loop_length > 0){
        ld->rw_head %= ld->loop_length;
      }else {
        ld->rw_head %= ld->loop_buf_size;
      }
    }
  }
}

void destroy_looper(struct node *self) {
  destroy_inlets(self);
  destroy_outlets(self);
  looper_data *ld = self->data;
  free(ld->buf);
  free(self->data);
}

node *new_looper(const patch *p) {
  node *n = malloc(sizeof(node));
  n->data = new_looper_data(p);
  n->last_visited = -1;
  n->process = &process_looper;
  n->destroy = &destroy_looper;
  n->num_inlets = 2;
  n->inlets = malloc(sizeof(inlet) * n->num_inlets);
  n->inlets[0] = new_inlet(p->audio_opts.buf_size, "in", 0.0);
  n->inlets[1] = new_inlet(p->audio_opts.buf_size, "trig", 0.0);
  n->num_outlets = 1;
  n->outlets = malloc(sizeof(outlet) * n->num_outlets);
  n->outlets[0] = new_outlet(p->audio_opts.buf_size, "out");
  return n;
}
