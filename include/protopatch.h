#include <stdlib.h>
#include <stdio.h>
#define TABLE_SIZE 64
#define MAX_NODES 2048

typedef struct {
  unsigned int buf_size;
  unsigned int sample_rate;
  unsigned int hw_in_channels;
  unsigned int hw_out_channels;
} audio_options;

typedef struct connection {
  struct connection *next;
  unsigned int in_node_id;
  unsigned int inlet_id;
} connection;

typedef struct {
  float *buf;
  int buf_size;
  int num_connections;
  char *name;
} inlet;

typedef struct {
  float *buf;
  int buf_size;
  int num_connections;
  connection *connections;
  char *name;
} outlet;

typedef struct {
  float val;
  char *name;
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
} node;

typedef struct node_list_elem {
  struct node_list_elem *prev;
  struct node_list_elem *next;
  node *node;
} node_list_elem;

typedef node_list_elem** node_table;

struct int_stack {
  int stk[MAX_NODES];
  int top;
};

typedef struct patch {
  //hashtable of nodes
  node_table table;
  struct int_stack order;
  //thing representing connections
  int next_id; //should monotonically increase
  int num_nodes; //dunno if I need this
  audio_options audio_opts;
  //it's kind of confusing, but hw_inlets are the audio outputs
  //hw_outlets are the audio inputs
  inlet *hw_inlets; //re-initing patch with different audio-options would be super annoying... but would kind of justify making inlets/outlets external to nodes
  int num_hw_inlets;
  outlet *hw_outlets;
  int num_hw_outlets;
} patch;

inlet new_inlet(int buf_size, char *name);

outlet new_outlet(int buf_size, char *name);

patch *new_patch();

void destroy_inlets(node *n);

void destroy_outlets(node *n);

void add_node(patch *p, node *n);

node *get_node(const patch *p, unsigned int id);

void free_patch(patch *p);

//TODO error handling
void pp_connect(patch *p, unsigned int out_node_id, unsigned int outlet_id,
             unsigned int in_node_id, unsigned int inlet_id);

void sort_patch(patch *p);

void process_patch(patch *p);

//deffo want a limiter on dac eventually
//dac can be a real node, but its inlets/outlets should just be pointers to inlets/outlets in patch struct
//

//TODO: remove these; want a standardized way to add nodes and set controls by name
node *new_sin_osc(patch *p);
node *new_dac(patch *p);
typedef struct {
  float amp;
  float phase;
  float freq;
  float phase_incr;
} sin_data;
