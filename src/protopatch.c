#include "protopatch.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include "tinyosc.h"

TinyPipe rt_consumer_pipe;

enum msg_type {
  ADD_NODE,
  DELETE_NODE,
  CONNECT,
  DISCONNECT,
};

struct rt_msg {
  enum msg_type type;
  node *node;
};

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
/*
node_list_elem *new_node_list_elem() {
  node_list_elem *elem = malloc(sizeof(node_list_elem));
  elem->prev = NULL;
  elem->next = NULL;
  elem->node = NULL;
  return elem;
}
*/

void free_node_list(node *n) {
  node *tmp;
  while (n) {
    tmp = n;
    n = n->next;
    if(tmp) {
      tmp->destroy(tmp);
      free(tmp);
    }
  }
}

void add_node(patch *p, node *n) {
  n->id = p->next_id++;
  printf("adding node with id %d\n", n->id);
  p->num_nodes++;

  unsigned int idx = n->id % TABLE_SIZE;

  //add to table
  node *head = p->table[idx];
  //if it's the first thing ever, just set head
  if(!head) {
    printf("setting head\n");
    head = n;
    p->table[idx] = head;
    return;
  }

  //otherwise push it onto the stack
  printf("not setting head, just pushing on stack\n");
  n->next = head;
  head->prev = n;
  p->table[idx] = n;
}

void free_node_table(node_table table) {
  for(int i = 0; i < TABLE_SIZE; i++) {
    free_node_list(table[i]);
  }
  free(table);
}

node *new_node(const patch *p, int num_inlets, int num_outlets){
  assert(num_inlets >= 0);
  assert(num_outlets >= 0);
  node *n = malloc(sizeof(node));
  n->last_visited = -1;
  n->num_inlets = num_inlets;
  n->num_controls = 0;
  if(num_inlets > 0) n->inlets = malloc(sizeof(inlet) * num_inlets);
  for(int i=0; i < num_inlets; i++){
    inlet *in = &n->inlets[i];
    in->buf = calloc(p->audio_opts.buf_size, sizeof(float));
    printf("the buf_size %d\n", p->audio_opts.buf_size);
    in->buf_size = p->audio_opts.buf_size;
    in->num_connections = 0;
  }

  n->num_outlets = num_outlets;
  if(num_outlets > 0) n->outlets = malloc(sizeof(outlet) * num_outlets);
  for(int i=0; i < num_outlets; i++){
    outlet *out = &n->outlets[i];
    out->buf = calloc(p->audio_opts.buf_size, sizeof(float));
    printf("the buf_size %d\n", p->audio_opts.buf_size);
    out->buf_size = p->audio_opts.buf_size;
    out->num_connections = 0;
    out->connections = NULL;
  }

  n->prev = NULL;
  n->next = NULL;
  return n;
}

//void destroy_node

void init_inlet(node *n, int idx, char *name, float default_val){
  assert(idx >= 0);
  assert(idx <= n->num_inlets);
  n->inlets[idx].name = name;
  n->inlets[idx].val = default_val;
}

void init_outlet(node *n, int idx, char *name){
  assert(idx >= 0);
  assert(idx <= n->num_outlets);
  n->outlets[idx].name = name;
}

//init_outlet
patch *new_patch(audio_options audio_opts) {
  patch *p = malloc(sizeof(patch));
  p->table = malloc(sizeof(node*) * TABLE_SIZE);//TODO this doesn't really need to be on the heap
  for(int i = 0; i < TABLE_SIZE; i++) p->table[i] = NULL;
  p->num_nodes = 0;
  p->next_id = 0;
  p->audio_opts = audio_opts;

  p->hw_inlets = malloc(sizeof(inlet) * audio_opts.hw_out_channels);
  for(int i = 0; i < audio_opts.hw_in_channels; i++){
    char *name = malloc(2);
    name[0] = i;
    name[1] = '\0';
    p->hw_inlets[i].buf = calloc(audio_opts.buf_size, sizeof(float));
    p->hw_inlets[i].buf_size = audio_opts.buf_size;
    p->hw_inlets[i].num_connections = 0;
    p->hw_inlets[i].name = name;
    p->hw_inlets[i].val = 0.0;
  }

  p->hw_outlets = malloc(sizeof(outlet) * audio_opts.hw_in_channels);
  for(int i = 0; i < audio_opts.hw_out_channels; i++){
    char *name = malloc(2);
    name[0] = i;
    name[1] = '\0';
    p->hw_outlets[i].buf = calloc(audio_opts.buf_size, sizeof(float));
    p->hw_outlets[i].buf_size = audio_opts.buf_size;
    p->hw_outlets[i].num_connections = 0;
    p->hw_outlets[i].connections = NULL;
    p->hw_outlets[i].name = name;
  }

#ifdef BELA
  p->digital_outlets = malloc(sizeof(outlet) * audio_opts.digital_channels);
  for(int i = 0; i < audio_opts.digital_channels; i++){
    char *name = malloc(2);
    name[0] = i;
    name[1] = '\0';
    //asprintf(&name, "digital:%d", i);
    p->digital_outlets[i] = new_outlet(audio_opts.digital_frames, name);//TODO: fix/free the name
  }
#endif
  return p;
}

void add_connection(outlet *out, inlet *in, unsigned int in_node_id, unsigned int inlet_id) {
  //TODO detect cycles
  connection *ptr = out->connections;
  if (!ptr) {
    out->num_connections++;
    in->num_connections++;
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
  in->num_connections++;
  ptr = malloc(sizeof(connection));
  ptr->next = NULL;
  ptr->in_node_id = in_node_id;
  ptr->inlet_id = inlet_id;
  prev->next = ptr;
}

node *get_node(const patch *p, unsigned int id) {
  node *n = p->table[id % TABLE_SIZE];
  while(n) {
    if(n->id == id) return n;
    n = n->next;
  }
  return NULL;
}

void free_patch(patch *p) {
  //free each node
  free_node_table(p->table);
  for(int i = 0; i < p->audio_opts.hw_out_channels; i++) {
    free(p->hw_inlets[i].buf);
  }
  free(p->hw_inlets);

  for(int i = 0; i < p->audio_opts.hw_in_channels; i++) {
    free(p->hw_outlets[i].buf);
  }
  free(p->hw_outlets);
  free(p);
}

void pp_connect(patch *p, unsigned int out_node_id, unsigned int outlet_id,
             unsigned int in_node_id, unsigned int inlet_id) {
  node *out = get_node(p, out_node_id);
  node *in = get_node(p, in_node_id);
  if (out && in && inlet_id < in->num_inlets
      && outlet_id < out->num_outlets) {
    add_connection(&out->outlets[outlet_id], &in->inlets[inlet_id], in_node_id, inlet_id);
  }
}

void dfs_visit(const patch *p, unsigned int generation, unsigned int node_id, struct int_stack *s) {
  node *n = get_node(p, node_id);
  n->last_visited = generation;
  for(int i = 0; i < n->num_outlets; i++) {
    connection *conn = n->outlets[i].connections;
    while(conn) {
      node *in = get_node(p, conn->in_node_id);
      if(in->last_visited != generation) {
        dfs_visit(p, generation, conn->in_node_id, s);
      }
      conn = conn->next;
    }
  }
  //push onto stack
  s->stk[s->top] = node_id;
  s->top++;
}

void sort_patch(patch *p) {
  static int generation = -1;
  generation++;
  //TODO maybe just use the stack on the patch, don't allocate a new one
  struct int_stack ordered_nodes = {.top = 0, .stk= {0}};
  for(int i = 0; i < TABLE_SIZE; i++) {
    node *n = p->table[i];
    while(n) {
      if(n->last_visited != generation) {
        dfs_visit(p, generation, n->id, &ordered_nodes);
      }
      n = n->next;
    }
  }
  ordered_nodes.top--;
  p->order = ordered_nodes;
}

void zero_all_inlets(const patch *p, struct int_stack s) {
  while(s.top >= 0) {
    node *n = get_node(p, s.stk[s.top]);
    for(int i = 0; i < n->num_inlets; i++) {
      inlet in = n->inlets[i];
      for(int j = 0; j < in.buf_size; j++) {
        in.buf[j] = 0.f;
      }
    }
    s.top--;
  }
}

void handle_rt_msg(patch *p, struct rt_msg *msg){
  switch(msg->type){
    case ADD_NODE:
      add_node(p, msg->node);
      printf("the new node id %d\n", msg->node->id);
      pp_connect(p, msg->node->id, 0, 0, 0);
      pp_connect(p, msg->node->id, 0, 0, 1);
      sort_patch(p);
      printf("yay! added a sin node with id: %d\n", msg->node->id);
      break;
    case DELETE_NODE:
      printf("yay! deleting node\n");
      break;
    case CONNECT:
      printf("yay! connecting node\n");
      break;
    case DISCONNECT:
      printf("yay! disconnecting node\n");
      break;
    default:
      printf("handle_rt_msg: unrecognized msg_type\n");//TODO make some kind of exit handler
  }
}

void process_patch(patch *p) {
  while(tpipe_hasData(&rt_consumer_pipe)){
    int len = 0;
    char *buf = NULL;
    buf = tpipe_getReadBuffer(&rt_consumer_pipe, &len);
    assert(len > 0);
    assert(buf != NULL);
    handle_rt_msg(p, (struct rt_msg*) buf);
    tpipe_consume(&rt_consumer_pipe);
  }

  zero_all_inlets(p, p->order);
  int top = p->order.top;
  while(top >= 0) {
    node *n = get_node(p, p->order.stk[top]);
    n->process(n);

    //add outlet contents to connected inlets
    for(int i = 0; i < n->num_outlets; i++) {
      outlet out = n->outlets[i];
      connection *c = out.connections;
      while(c) {
        node *in_node = get_node(p, c->in_node_id);
        inlet in = in_node->inlets[c->inlet_id];
        for(int idx = 0; idx < out.buf_size; idx++) {
          in.buf[idx] += out.buf[idx];
        }
        /* resampling - commenting because I dunno if I really even want different buffer sizes
        if(out.buf_size == in.buf_size){
          for(int idx = 0; idx < out.buf_size; idx++) {
            in.buf[idx] += out.buf[idx];
          }
        }else if(out.buf_size < in.buf_size){
          //upsampling by just repeating samples - "ZOH zero-order-hold" TODO: learn dsp and do zeros + lowpass filtering
          //making the assumption that the buffer sizes are always powers of 2
          unsigned int shift_amount = 1;
          while(in.buf_size >> shift_amount != out.buf_size) shift_amount++;
          for(int in_idx = 0; in_idx < in.buf_size; in_idx++){
            in.buf[in_idx] += out.buf[in_idx >> shift_amount];
          }
        }else{
          //downsampling/decimation by skipping samples - TODO: learn dsp and do a nice lowpass to prevent aliasing
          unsigned int shift_amount = 1;
          while(out.buf_size >> shift_amount != in.buf_size) shift_amount++;
          for(int in_idx = 0; in_idx < in.buf_size; in_idx++){
            in.buf[in_idx] += out.buf[in_idx << shift_amount];
          }
        }
        */
        c = c->next;
      }
    }
    top--;
  }
}

void set_control(node *n, char *ctl_name, float val) {
  for(int i = 0; i < n->num_inlets; i++) {
    inlet *in = &n->inlets[i];
    if(strcmp(in->name, ctl_name) == 0) {
      in->val = val;
      return;
    }
  }
}

void no_op(struct node *self) {return;}

void handle_osc_message(const patch *p, tosc_message *osc){
  char *address = tosc_getAddress(osc);
  if(strncmp(address, "/node/add", strlen("/node/add")) == 0){
    const char *type = tosc_getNextString(osc);
    printf("got a node add message: %s\n", type);
    node *n = NULL;
    if(strcmp(type, "sin") == 0){
      printf("making a new sin node\n");
      n = new_sin_osc(p);
    }
    //write to rt thread
    struct rt_msg msg = {.type = ADD_NODE, .node = n};
    assert(tpipe_write(&rt_consumer_pipe, (char *)&msg, sizeof(struct rt_msg)) == 1);
  }
}

void run_osc_server(const patch *p){
  const int fd = socket(AF_INET, SOCK_DGRAM, 0); //ip v4 datagram socket
  fcntl(fd, F_SETFL, O_NONBLOCK); // set the socket to non-blocking
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(9000);
  sin.sin_addr.s_addr = INADDR_ANY;
  bind(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
  printf("tinyosc is now listening on port 9000.\n");
  printf("Press Ctrl+C to stop.\n");

  char buffer[2048];

  while (keepRunning) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);
    struct timeval timeout = {1, 0}; // select times out after 1 second
    if (select(fd+1, &readSet, NULL, NULL, &timeout) > 0) {
      struct sockaddr sa; // can be safely cast to sockaddr_in
      socklen_t sa_len = sizeof(struct sockaddr_in);
      int len = 0;
      while ((len = (int) recvfrom(fd, buffer, sizeof(buffer), 0, &sa, &sa_len)) > 0) {
        if (tosc_isBundle(buffer)) {
          tosc_bundle bundle;
          tosc_parseBundle(&bundle, buffer, len);
          /* const uint64_t timetag = tosc_getTimetag(&bundle); */
          tosc_message osc;
          while (tosc_getNextMessage(&bundle, &osc)) {
            handle_osc_message(p, &osc);
          }
        } else {
          tosc_message osc;
          tosc_parseMessage(&osc, buffer, len);
          handle_osc_message(p, &osc);
        }
      }
    }
  }
}
