#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define TABLE_SIZE 64
#define MAX_NODES 2048
//process loop
  //sort nodes
  //for each n in nodes
    //process n, move to float out
    //copy to each connected inlet

//process node
  //node has array of (named?) inlets, which can be written to
  //produces array of (named?) outlets. or maybe just has them
  //

//buffersize should probably be passed in to each constructor, may definitely also want things like samplerate
//but tbh these things are global and should not change during runtime
struct graph;

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

typedef enum {
  SIN_OSC
} node_type;

typedef struct node {
  int id;
  node_type type;
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

void free_connections(outlet out) {
  connection *ptr = out.connections;
  connection *prev;
  while(ptr) {
    prev = ptr;
    ptr = ptr->next;
    free(prev);
  }
}

void destroy_inlets(node *n) {
  for(int i = 0; i < n->num_inlets; i++) {
    free(n->inlets[i].buf);
  }
  free(n->inlets);
}

void destroy_outlets(node *n) {
  for(int i = 0; i < n->num_outlets; i++) {
    free_connections(n->outlets[i]);
    free(n->outlets[i].buf);
  }
  free(n->outlets);
}

//TODO destroy_controls once I figure out what a control is

//would call a list element "node" but I'm already calling audio objects nodes
typedef struct list_elem {
  struct list_elem *prev;
  struct list_elem *next;
  node *node;
} list_elem;

list_elem *new_list_elem() {
  list_elem *elem = malloc(sizeof(list_elem));
  elem->prev = NULL;
  elem->next = NULL;
  elem->node = NULL;
  return elem;
}

list_elem *search_for_elem(list_elem *head, unsigned int node_id) {
  list_elem *ptr = head;
  while(ptr && ptr->node) {
    if(ptr->node->id == node_id) return ptr;
    ptr = ptr->next;
  }
  return NULL;
}
/*
void add_elem(list_elem **head, list_elem *elem) {
  if((*head)->node == NULL) {
    //if head's node is null, this is the first element we've added to the list
    *head = elem;
    free(head);
    return;
  }
  //always adding to head
  //not checking for dup elements, probably not going to do that
  (*head)->prev = elem;  
  elem->next = *head;
  *head = elem;
}

void remove_elem(list_elem **head, list_elem *elem) {
  if(!elem) return;
  if(*head == elem) {
    if (elem->next) {
      elem->next->prev = NULL;
      *head = elem->next;
      free(elem);
    }else {
      (*head)->node = NULL;
    }
    
    //TODO: node needs to be cleaned up, too
    return;
  }
  if(elem->prev) elem->prev->next = elem->next;
  if(elem->next) elem->next->prev = elem->prev;
  free(elem);
}
*/
void free_node_list(list_elem *ptr) {
  list_elem *tmp;
  while (ptr) {
    tmp = ptr;
    ptr = ptr->next;
    if(tmp->node) { 
      tmp->node->destroy(tmp->node);
      free(tmp->node);
    }
    free(tmp);
  }
}

void add_to_table(list_elem **nt, node *n) {
  unsigned int idx = n->id % TABLE_SIZE;
  list_elem *head = nt[idx];
  //if it's the first thing ever, just set head
  if(!head) {
    head = new_list_elem();
    head->node = n;
    nt[idx] = head;
    return;
  }

  //otherwise allocate new pointer and push it onto head of list
  list_elem *elem = new_list_elem();
  elem->node = n;
  elem->next = head;
  head->prev = elem;
//uhhh
  nt[idx] = elem;
}

/*
void remove_from_table(node_table *nt, int node_id) {
  list_elem **head = &nt->table[node_id % TABLE_SIZE];
  list_elem *elem = search_for_elem(*head, node_id);
  if (elem) remove_elem(head, elem);
}
*/

void free_node_table(list_elem **node_table) {
  for(int i = 0; i < TABLE_SIZE; i++) {
    free_node_list(node_table[i]);
  }
  free(node_table);
}

/*
void visit_whole_table(node_table *nt) {
  for(int i = 0; i < TABLE_SIZE; i++) {
    list_elem *ptr = nt->table[i];
    while (ptr && ptr->node) {
      printf("visiting node %d\n", ptr->node->id);
      ptr = ptr->next;
    }
  }
}
*/

typedef struct graph {
  //hashtable of nodes
  list_elem **node_table;
  //thing representing connections
  int next_id; //should monotonically increase
  int num_nodes; //dunno if I need this
  audio_options audio_opts;
  //it's kind of confusing, but hw_inlets are the audio outputs
  //hw_outlets are the audio inputs
  inlet *hw_inlets; //re-initing graph with different audio-options would be super annoying... but would kind of justify making inlets/outlets external to nodes
  int num_hw_inlets;
  outlet *hw_outlets;
  int num_hw_outlets;
  //inlet *inlet_pool;
  //outlet *inlet_pool; //inlets/outlets would grab these and free them during processing
  //inlet needed when copy from 
} graph;

//new_graph
//
//typedef node_data void* ?
//should a node have static control values?
//yeah I think so, maybe even controls that have no inlets
//but how does the node know if its inlets are connected?
//store num connections? when 0 read from control
//
//
//int add_node(struct graph g, node_type, default parameters?)
//
inlet new_inlet(int buf_size, char *name) {
  inlet i;
  i.buf = calloc(buf_size, sizeof(float));
  i.buf_size = buf_size;
  i.name = name;
  return i;
}

outlet new_outlet(int buf_size, char *name) {
  outlet o;
  o.buf = calloc(buf_size, sizeof(float));
  o.buf_size = buf_size;
  o.name = name;
  o.num_connections = 0;
  o.connections = NULL;
  return o;
}

graph *new_graph() {
  audio_options a = {.buf_size = 64, .sample_rate = 44100, 
                     .hw_in_channels = 2, .hw_out_channels = 2};
  graph *g = malloc(sizeof(graph));
  g->node_table = malloc(sizeof(list_elem *) * TABLE_SIZE);
  for(int i = 0; i < TABLE_SIZE; i++) g->node_table[i] = NULL;
  g->num_nodes = 0;
  g->next_id = 0;
  g->audio_opts = a;
  g->num_hw_inlets = 2;
  g->hw_inlets = malloc(sizeof(inlet) * 2);
  g->hw_outlets = malloc(sizeof(outlet) * 2);
  g->hw_inlets[0] = new_inlet(64, "left");
  g->hw_inlets[1] = new_inlet(64, "right");
  g->num_hw_outlets = 2;
  g->hw_outlets[0] = new_outlet(64, "left");
  g->hw_outlets[1] = new_outlet(64, "right");
  return g;
}

//sample nodes
//sin
//dac
//adc


void add_connection(outlet *out, unsigned int in_node_id, unsigned int inlet_id) {
  //TODO detect cycles
  connection *ptr = out->connections;
  if (!ptr) {
    out->num_connections++;
    ptr = malloc(sizeof(connection));
    ptr->next = NULL;
    ptr->in_node_id = in_node_id;
    ptr->inlet_id = inlet_id;
    out->connections = ptr;
    return;
  }
  connection *prev = ptr;
  while(ptr) {
    if(ptr->in_node_id == in_node_id && ptr->inlet_id == inlet_id) return;
    prev = ptr;
    ptr = ptr->next;
  }
  out->num_connections++;
  ptr = malloc(sizeof(connection));
  ptr->next = NULL;
  ptr->in_node_id = in_node_id;
  ptr->inlet_id = inlet_id;
  prev->next = ptr; 
}

typedef struct {
  float phase;
  float freq;
  float inv_sr;
} sin_data;

void process_sin (struct node *self) {
  float *buf = self->outlets[0].buf;
  sin_data *data = self->data;
  for(int i = 0; i < self->outlets[0].buf_size; i++) {
    buf[i] = sinf(data->phase);
    data->phase += data->freq * data->inv_sr * 2.0 * M_PI;
  }
}

void process_dac (struct node *self) {
  //dac does not need to be processed
  //
  //float *left = self->inlets[0].buf;
  //float *right = self->inlets[1].buf;
  //float *hw_left = g->hw_inlets[0].buf;
  //float *hw_right = g->hw_inlets[0].buf;
  //for(int i = 0; i < g->audio_opts.buf_size; i++) {
  //  hw_left[i] += left[i];
  //  hw_right[i] += right[i];
  //}
}

void destroy_dac (struct node *self) {
  return;
}

node *new_dac(graph *g) {
  node *n = malloc(sizeof(node));
  //n->type = 
  n->process = &process_dac;
  n->destroy = &destroy_dac;
  n->num_inlets = g->num_hw_inlets;
  n->num_outlets = 0;
  n->num_controls = 0;
  n->last_visited = -1;
  n->inlets = g->hw_inlets;
  return n;
}

void destroy_sin (struct node *self) {
  destroy_inlets(self);
  destroy_outlets(self);
  free((sin_data*)self->data);
}

sin_data *new_sin_data(graph *g){
  sin_data *d = malloc(sizeof(sin_data));
  d->freq = 540.0;
  d->phase = 0.0;
  d->inv_sr = (1.0 / g->audio_opts.sample_rate);
  return d;
}

node *new_sin_osc(graph *g) {
  node *n = malloc(sizeof(node));
  n->type = SIN_OSC;
  n->data = new_sin_data(g);
  n->process = &process_sin;
  n->destroy = &destroy_sin;
  n->num_inlets = 2;//TODO create_inlet convenience function
  n->num_outlets = 1;
  n->num_controls = 0;
  n->last_visited = -1;
  n->inlets = malloc(sizeof(inlet) * 2);
  n->inlets[0] = new_inlet(64, "phase");
  n->inlets[1] = new_inlet(64, "freq");
  n->outlets = malloc(sizeof(outlet));
  n->outlets[0] = new_outlet(64, "out");
  return n;
}


//
//dispatch - maybe don't need this yet, but will need something like it for handling OSC commands
//TODO switch case dispatch is maybe negligibly faster maybe
/*
node *new_node(node_type type) {
  if(type == SIN_OSC) return new_sin_osc();
  else return NULL;
}
*/

//m/aybe be able to init node with data?
void add_node(graph *g, node *n) {
  n->id = g->next_id++;
  g->num_nodes++;
  add_to_table(g->node_table, n);
}

node *get_node(graph *g, unsigned int id) {
  list_elem *e = search_for_elem(g->node_table[id % TABLE_SIZE], id);
  if (e) return e->node;
  return NULL;
}

void free_graph(graph *g) {
  //free each node
  free_node_table(g->node_table);
  for(int i = 0; i < g->num_hw_inlets; i++) {
    free(g->hw_inlets[i].buf);
  }
  free(g->hw_inlets);

  for(int i = 0; i < g->num_hw_outlets; i++) {
    free(g->hw_outlets[i].buf);
  }
  free(g->hw_outlets);
  free(g);
}

//constructs node based on node type dispatch
//assigns id
//adds node to hashtable
//returns id of newly created node

//TODO error handling
void connect(graph *g, unsigned int out_node_id, unsigned int outlet_id, 
             unsigned int in_node_id, unsigned int inlet_id) {
  node *out = get_node(g, out_node_id);
  node *in = get_node(g, in_node_id);
  if (out && in && inlet_id < in->num_inlets 
      && outlet_id < out->num_outlets) {
    add_connection(&out->outlets[outlet_id], in_node_id, inlet_id);
  }
}

struct int_stack {
  int stk[MAX_NODES];
  int top;
};

void dfs_visit(graph *g, unsigned int generation, unsigned int node_id, struct int_stack *s) {
  node *n = get_node(g, node_id);
  n->last_visited = generation;
  for(int i = 0; i < n->num_outlets; i++) {
    connection *conn = n->outlets[i].connections;
    while(conn) {
      node *in = get_node(g, conn->in_node_id);
      if(in->last_visited != generation) {
        dfs_visit(g, generation, conn->in_node_id, s);
      }
      conn = conn->next;
    }
  }
  //push onto stack
  s->stk[s->top] = node_id;
  s->top++;
}

struct int_stack sort_graph(graph *g) {
  static int generation = -1;
  generation++;
  //should return a linked-list of node ids in order that they should be processed
  struct int_stack ordered_nodes = {.top = 0, .stk= {0}};
  //int *visited = malloc(sizeof(int) * g->num_nodes);
  //for(int i = 0; i < g->num_nodes; i++) visited[i] = -1;
  for(int i = 0; i < TABLE_SIZE; i++) {
    list_elem *ptr = g->node_table[i];
    while(ptr && ptr->node) {
      //if unvisited, visit
      if(ptr->node->last_visited != generation) {
        dfs_visit(g, generation, ptr->node->id, &ordered_nodes);
      }
      ptr = ptr->next;
    }
  }
  ordered_nodes.top--;
  return ordered_nodes;
}

void process_graph(graph *g, struct int_stack s) {
  while(s.top >= 0) {
    //process node
    node *n = get_node(g, s.stk[s.top]);
    n->process(n);

    //add outlet contents to connected inlets
    for(int i = 0; i < n->num_outlets; i++) {
      outlet o = n->outlets[i];
      connection *c = o.connections;
      while(c) {
        node *in_node = get_node(g, c->in_node_id);
        inlet in = in_node->inlets[c->inlet_id];
        for(int j = 0; j < g->audio_opts.buf_size; j++) {
          in.buf[j] += o.buf[j];
        }
        c = c->next;
      }
    }
    s.top--;
  }
  inlet output = g->hw_inlets[1];
  for(int i = 0; i < g->audio_opts.buf_size; i++) {
    fwrite(&output.buf[i], 1, sizeof(float), stdout);
  }
}


//bool connect(int out_node_id, int outlet_id, int in_node_id, int inlet_id)
//ensure variables actually exist, test for cycles
  //happy path - increment num_connections on inlet, update connection
  //
//
int main() {
  graph *g = new_graph();
  node *sin = new_sin_osc(g);
  //node *sin2 = new_sin_osc();
  node *dac = new_dac(g);
  add_node(g, dac);
  add_node(g, sin);
  //add_node(g, sin2);
  connect(g, sin->id, 0, dac->id, 0);
  connect(g, sin->id, 0, dac->id, 1);
  //connect(g, sin->id, 0, sin2->id, 1);
  //connect(g, sin->id, 0, sin2->id, 0);
  //connect(g, sin->id, 0, sin2->id, 1);
  //connect(g, sin->id, 0, sin2->id, 1);
  //connect(g, sin->id, 0, sin2->id, 0);
  //connect(g, sin->id, 0, sin2->id, 0);

  //head->node = sin;

  //list_elem *l2 = new_list_elem();
  //l2->node = sin2;


  //add_elem(&head, l2);

  //printf("not null? %d\n", search_for_elem(head, 0));
  //printf("not null? %d\n", search_for_elem(head, 1));

  //remove_elem(&head, search_for_elem(head, 1));
  //printf("null? %d\n", search_for_elem(head, 1));

  //node *sin3 = new_sin_osc();
  //node *sin4 = new_sin_osc();
  //node *sin5 = new_sin_osc();
  //sin->id = 0;
  //sin2->id = 1;
  //sin3->id = 2;
  //sin4->id = 3;
  //sin5->id = 4;

  //node_table nt = new_node_table();
  //add_to_table(&nt, sin);
  //add_to_table(&nt, sin2);
  //add_to_table(&nt, sin3);
  //add_to_table(&nt, sin4);
  //add_to_table(&nt, sin5);

  ////visit_whole_table(&nt);
  //remove_from_table(&nt, 0);
  //visit_whole_table(&nt);
  //remove_from_table(&nt, 3);
  //remove_from_table(&nt, 2);
  //remove_from_table(&nt, 1);
  //remove_from_table(&nt, 4);
  //visit_whole_table(&nt);
  //
  struct int_stack s = sort_graph(g);
  for(int i = 0; i < 1000; i++){
    process_graph(g, s);
  }
  free_graph(g);
  //free(sin);
  //free(sin2);
  //free(sin2);
}

//deffo want a limiter on dac eventually
//dac can be a real node, but its inlets/outlets should just be pointers to inlets/outlets in graph struct

//
