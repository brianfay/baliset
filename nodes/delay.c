#include "protopatch.h"

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
  inlet i_delay_time = self->inlets[1];
  inlet i_feedback = self->inlets[2];

  for(int i = 0; i < o_out.buf_size; i++) {
    float in = in_buf[i];
    float delay_time = (i_delay_time.num_connections > 0) ? i_delay_time.buf[i] : i_delay_time.val;
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

    float feedback = (i_feedback.num_connections > 0) ? i_feedback.buf[i] : i_feedback.val;
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
  destroy_inlets(self);
  destroy_outlets(self);
  delay_data *data = (delay_data*) self->data;
  free(data->delay_line);
  free(data);
}

delay_data *new_delay_data(const patch *p){
  delay_data *data = malloc(sizeof(delay_data));
  data->max_delay_seconds = 3;
  data->sample_rate = p->audio_opts.sample_rate;
  data->delay_line_size = data->sample_rate * data->max_delay_seconds;
  data->delay_line = calloc(data->delay_line_size, sizeof(float));
  return data;
}

node *new_delay(const patch *p) {
  printf("in new_delay\n");
  node *n = malloc(sizeof(node));
  n->last_visited = -1;
  n->data = new_delay_data(p);
  n->process = &process_delay;
  n->destroy = &destroy_delay;
  n->num_inlets = 3;
  n->inlets = malloc(sizeof(inlet) * n->num_inlets);
  n->inlets[0] = new_inlet(p->audio_opts.buf_size, "in", 0.0);
  n->inlets[1] = new_inlet(p->audio_opts.buf_size, "delay_time", 0.25);
  n->inlets[2] = new_inlet(p->audio_opts.buf_size, "feedback", 0.6);
  n->num_outlets = 1;
  n->outlets = malloc(sizeof(outlet) * n->num_outlets);
  n->outlets[0] = new_outlet(p->audio_opts.buf_size, "out");
  return n;
}
