#ifndef BALISET_GRAPH_H
#define BALISET_GRAPH_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
  //#include <string.h>

#define TABLE_SIZE 64
#define MAX_NODES 2048
#define WAVETABLE_SIZE 1024

typedef struct {
  unsigned int buf_size;
  unsigned int sample_rate;
  unsigned int hw_in_channels;
  unsigned int hw_out_channels;
#ifdef BELA
  unsigned int digital_channels;
  unsigned int digital_frames;
#endif
} blst_audio_options;

typedef struct connection {
  struct connection *next;
  unsigned int node_id;
  unsigned int io_id;
} blst_connection;

typedef struct {
  float *buf;
  int buf_size;
  int num_connections;
  blst_connection *connections;
} blst_inlet;

typedef struct {
  float *buf;
  int buf_size;
  int num_connections;
  blst_connection *connections;
} blst_outlet;

typedef struct blst_control {
  float val;
  void (*set_control) (struct blst_control *self, float val);
} blst_control;

typedef struct blst_node {
  int id;
  void *data;
  blst_inlet *inlets;
  unsigned int num_inlets;
  blst_outlet *outlets;
  unsigned int num_outlets;
  blst_control *controls;
  unsigned int num_controls;
  unsigned int last_visited;// generation/timestamp thing
  void (*process) (struct blst_node *self);
  void (*destroy) (struct blst_node *self);
  //keeping doubly-linked lists of nodes inside of a hash table, so we need references to prev/next
  struct blst_node *prev;
  struct blst_node *next;
} blst_node;

typedef blst_node** blst_node_table;

struct blst_int_stack {
  int stk[MAX_NODES];
  int top;
};

typedef struct {
  float sine_table[WAVETABLE_SIZE];
} blst_wavetables;

typedef struct blst_patch {
  blst_audio_options audio_opts;
  //hashtable of nodes
  blst_node_table table;
  struct blst_int_stack order;
  int next_id; //should monotonically increase
  //it's kind of confusing, but hw_inlets are the audio outputs
  //hw_outlets are the audio inputs
  blst_inlet *hw_inlets;
#ifdef BELA
  blst_outlet *digital_outlets;
  //TODO might want digital inlets, and analog inlets/outlets
#endif
  blst_outlet *hw_outlets;
  blst_wavetables wavetables;
} blst_patch;

struct blst_disconnect_pair {
  blst_connection *in_conn;
  blst_connection *out_conn;
};

blst_node *blst_new_node(const blst_patch *p, const char *type);

blst_node *blst_init_node(const blst_patch *p, int num_inlets, int num_outlets, int num_controls);

void blst_init_inlet(const blst_patch *p, blst_node *n, int idx);

void blst_init_outlet(const blst_patch *p, blst_node *n, int idx);

blst_patch *blst_new_patch(blst_audio_options audio_opts);

void blst_destroy_inlets(blst_node *n);

void blst_destroy_outlets(blst_node *n);

void blst_add_node(blst_patch *p, blst_node *n);

void blst_remove_node(blst_patch *p, blst_node *n);//doesn't actually free the memory, hang on to that pointer!

blst_node *blst_get_node(const blst_patch *p, unsigned int id);

void blst_free_node(blst_node *n);

void blst_free_patch(blst_patch *p);

void blst_add_connection(blst_patch *p, blst_connection *out_conn, blst_connection *in_conn);

//public
void blst_connect(blst_patch *p, unsigned int out_node_id, unsigned int outlet_id,
                  unsigned int in_node_id, unsigned int inlet_id);

//private
struct blst_disconnect_pair blst_disconnect(const blst_patch *p, int out_node_id, int outlet_idx, int in_node_id, int inlet_idx);

//private
void blst_sort_patch(blst_patch *p);

//public
void blst_process(const blst_patch *p);

//public
void blst_set_control(blst_node *n, int ctl_id, float val);

//TODO: I dislike putting these all in the top-level header but am having trouble finding a cleaner approach in C
blst_node *blst_new_sin_osc(const blst_patch *p);
blst_node *blst_new_adc(const blst_patch *p);
blst_node *blst_new_buthp(const blst_patch *p);
blst_node *blst_new_dac(const blst_patch *p);
blst_node *blst_new_delay(const blst_patch *p);
blst_node *blst_new_dist(const blst_patch *p);
blst_node *blst_new_float(const blst_patch *p);
blst_node *blst_new_flip_flop(const blst_patch *p);
blst_node *blst_new_hip(const blst_patch *p);
blst_node *blst_new_mul(const blst_patch *p);
blst_node *blst_new_noise_gate(const blst_patch *p);
blst_node *blst_new_looper(const blst_patch *p);
#ifdef BELA
blst_node *blst_new_digiread(const blst_patch *p);
#endif

#ifdef __cplusplus
}
#endif

#endif //BALISET_GRAPH
