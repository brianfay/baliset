#include "baliset_graph.h"

typedef struct {
  float *delay_line;
  unsigned int max_delay_seconds;
  unsigned int delay_line_size;
  unsigned int write_head;
  unsigned int sample_rate;
} delay_data;

void process_delay(struct node *self) {
  delay_data *data = self->data;
  outlet o_out = self->outlets[0];
  float *out_buf = o_out.buf;
  float *in_buf = self->inlets[0].buf;
  float *delay_line = data->delay_line;
  /* inlet i_delay_time = self->inlets[1]; */
  /* inlet i_feedback = self->inlets[2]; */
  float delay_time = self->controls[0].val;
  float feedback = self->controls[1].val;

  for(int i = 0; i < o_out.buf_size; i++) {
    float in = in_buf[i];
    if(delay_time < 0.0) delay_time = 0.0;
    if(delay_time > data->max_delay_seconds) delay_time = (float)data->max_delay_seconds;
    //TODO lerp, lopass filter?
    float delay_samples = delay_time * data->sample_rate;
    int read_head = data->write_head - (int)delay_samples;
    if(read_head < 0) {read_head += data->delay_line_size;}
    //could be negative, could be over delay_line_size
    //write the input plus the delayed signal
    float out = in + delay_line[read_head];
    out_buf[i] = out;

    if(feedback < 0.0) feedback = 0.0;
    if(feedback > 1.0) feedback = 1.0;
    /* //write the input to the delay_line */
    delay_line[data->write_head] = out * feedback;
    /* //move write_head */
    data->write_head++;
    data->write_head = data->write_head % data->delay_line_size;
  }
}

void destroy_delay(struct node *self) {
  delay_data *d = self->data;
  free(d->delay_line);
}

delay_data *new_delay_data(const patch *p){
  delay_data *data = malloc(sizeof(delay_data));
  data->max_delay_seconds = 3;
  data->sample_rate = p->audio_opts.sample_rate;
  data->delay_line_size = data->sample_rate * data->max_delay_seconds;
  data->delay_line = calloc(data->delay_line_size, sizeof(float));
  data->write_head = 0;
  return data;
}

node *new_delay(const patch *p) {
  node *n = init_node(p, 1, 1, 2);
  n->data = new_delay_data(p);
  n->process = &process_delay;
  n->destroy = &destroy_delay;

  //in
  init_inlet(p, n, 0);
  /* init_inlet(n, 1, "delay_time", 0.25); */
  /* init_inlet(n, 2, "feedback", 0.6); */

  //delay_time
  n->controls[0].val = 0.25;
  //feedback
  n->controls[1].val = 0.6;

  //out
  init_outlet(p, n, 0);
  return n;
}
