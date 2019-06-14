#include "baliset.h"

typedef struct {
  float *buf;
  unsigned int recording;
  float rw_head;
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
  float ctl_rate = self->controls[0].val;
  float ctl_trig = self->controls[1].val;
  for(int i=0; i < o_out.buf_size; i++) {
    float new_trig_samp = i_trig.num_connections > 0 ? trig_buf[i] : ctl_trig;
    if(ld->prev_trig_samp <= 0.0 && new_trig_samp > 0.0) {
      if(ld->recording) {ld->loop_length = ld->rw_head;}
      ld->loop_length %= ld->loop_buf_size;
      ld->rw_head = 0;
      ld->recording = (ld->recording + 1) % 2; //alternate between 0 and 1
    }
    ld->prev_trig_samp = new_trig_samp;

    if(ld->recording) {
      out_buf[i] = 0.0;
      ld->buf[(int)ld->rw_head] = in_buf[i];
      ld->rw_head++;
      if(ld->rw_head > ld->loop_buf_size) ld->rw_head -= ld->loop_buf_size;
      if(ld->rw_head < 0) ld->rw_head += ld->loop_buf_size;
    }else {
      float lerp_ratio = ld->rw_head - (int)ld->rw_head;
      float prev_sample = ld->buf[(int)ld->rw_head % ld->loop_buf_size];
      float next_sample = ld->buf[((int)ld->rw_head + 1 ) % ld->loop_buf_size];
      out_buf[i] = (1.0 - lerp_ratio) * prev_sample + (lerp_ratio * next_sample);
      //linear interpolation:
      //you have two points, you need a value in between, you guess the value by plotting a line between the points
      ld->rw_head += ctl_rate;
      if(ld->loop_length > 0){
        if(ld->rw_head > ld->loop_length) ld->rw_head -= ld->loop_length;
        if(ld->rw_head < 0) ld->rw_head += ld->loop_length;
      }else{
        if(ld->rw_head > ld->loop_buf_size) ld->rw_head -= (float)ld->loop_buf_size;
        if(ld->rw_head < 0) ld->rw_head += ld->loop_buf_size;
      }
    }
  }
}

void destroy_looper(struct node *self) {
  destroy_inlets(self);
  destroy_outlets(self);
  free(self->controls);
  looper_data *ld = self->data;
  free(ld->buf);
  free(self->data);
}

node *new_looper(const patch *p) {
  node *n = init_node(p, 2, 1, 2);
  n->data = new_looper_data(p);
  n->process = &process_looper;
  n->destroy = &destroy_looper;

  //in
  init_inlet(p, n, 0);
  //trig
  init_inlet(p, n, 1);

  //out
  init_outlet(p, n, 0);

  //rate
  n->controls[0].val = 1.0;
  //trig
  n->controls[1].val = 0.0;
  return n;
}
