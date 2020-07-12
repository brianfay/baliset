#ifdef __cplusplus
extern "C" {
#endif

#ifndef BALISET_H
#define BALISET_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "tinypipe.h"
#define TABLE_SIZE 64
#define MAX_NODES 2048

typedef struct {
  unsigned int buf_size;
  unsigned int sample_rate;
  unsigned int hw_in_channels;
  unsigned int hw_out_channels;
#ifdef BELA
  unsigned int digital_channels;
  unsigned int digital_frames;
#endif
} audio_options;

typedef struct connection {
  struct connection *next;
  unsigned int node_id;
  unsigned int io_id;
} connection;

typedef struct {
  float *buf;
  int buf_size;
  int num_connections;
  connection *connections;
} inlet;

typedef struct {
  float *buf;
  int buf_size;
  int num_connections;
  connection *connections;
} outlet;

typedef struct control {
  float val;
  void (*set_control) (struct control *self, float val);
} control;

//private data that will be different for different types of nodes
typedef void *node_data;

typedef struct node {
  int id;
  node_data data;
  inlet *inlets;
  unsigned int num_inlets;
  outlet *outlets;
  unsigned int num_outlets;
  control *controls;
  unsigned int num_controls;
  unsigned int last_visited;// generation/timestamp thing
  void (*process) (struct node *self);
  void (*destroy) (struct node *self);
  //keeping doubly-linked lists of nodes inside of a hash table, so we need references to prev/next
  struct node *prev;
  struct node *next;
} node;

typedef node** node_table;

struct int_stack {
  int stk[MAX_NODES];
  int top;
};

typedef struct patch {
  audio_options audio_opts;
  //hashtable of nodes
  node_table table;
  struct int_stack order;
  int next_id; //should monotonically increase
  //it's kind of confusing, but hw_inlets are the audio outputs
  //hw_outlets are the audio inputs
  inlet *hw_inlets;
#ifdef BELA
  outlet *digital_outlets;
  //TODO might want digital inlets, and analog inlets/outlets
#endif
  outlet *hw_outlets;
} patch;

//top level app state
typedef struct blst_system {
  audio_options audio_opts;
  patch *p;
  TinyPipe consumer_pipe, producer_pipe;
} blst_system;

node *new_node(const patch *p, const char *type);

node *init_node(const patch *p, int num_inlets, int num_outlets, int num_controls);

void init_inlet(const patch *p, node *n, int idx);

void init_outlet(const patch *p, node *n, int idx);

blst_system *new_blst_system(audio_options audio_opts);

patch *new_patch(audio_options audio_opts);

void destroy_inlets(node *n);

void destroy_outlets(node *n);

void add_node(patch *p, node *n);

void remove_node(patch *p, node *n);//doesn't actually free the memory, hang on to that pointer!

node *get_node(const patch *p, unsigned int id);

void free_node(node *n);

void free_patch(patch *p);

void free_blst_system(blst_system *p);

//TODO error handling
void blst_connect(patch *p, unsigned int out_node_id, unsigned int outlet_id,
             unsigned int in_node_id, unsigned int inlet_id);

void blst_disconnect(patch *p, unsigned int out_node_id,
                   unsigned int outlet_id, unsigned int in_node_id, unsigned int inlet_id);

void sort_patch(patch *p);

void blst_process(blst_system *bs);

void set_control(node *n, int ctl_id, float val);

//deffo want a limiter on dac eventually
//dac can be a real node, but its inlets/outlets should just be pointers to inlets/outlets in patch struct
//

void no_op(struct node *self);

void run_osc_server(blst_system *bs);
void stop_osc_server();

//TODO: I dislike putting these all in the top-level header but am having trouble finding a cleaner approach in C
node *new_sin_osc(const patch *p);
node *new_adc(const patch *p);
node *new_buthp(const patch *p);
node *new_dac(const patch *p);
node *new_gate_adc(const patch *p);
node *new_delay(const patch *p);
node *new_dist(const patch *p);
node *new_hip(const patch *p);
node *new_mul(const patch *p);
node *new_noise_gate(const patch *p);
node *new_looper(const patch *p);
#ifdef BELA
node *new_digiread(const patch *p);
#endif

#endif

#ifdef __cplusplus
}
#endif
