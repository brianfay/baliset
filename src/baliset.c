#include "baliset.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "tinyosc.h"

enum rt_msg_type {
  ADD_NODE,
  DELETE_NODE,
  CONNECT,
  DISCONNECT,
  CONTROL,
  REPLACE_PATCH,
};

enum non_rt_msg_type {
  FREE_PTR,
  FREE_NODE,
};

struct add_msg {
  node *node;
};

struct delete_msg {
  int32_t node_id;
};

struct control_msg {
  int32_t node_id;
  int32_t ctl_id;
  float val;
};

struct connect_msg {
  connection *in_conn;
  connection *out_conn;
};

struct disconnect_msg {
  int32_t out_node_id;
  int32_t outlet_idx;
  int32_t in_node_id;
  int32_t inlet_idx;
};

struct replace_patch_msg {
  int *done;
  patch *new_p;
  patch **old_p;
};

struct free_ptr_msg {
  void *ptr;
};

struct free_node_msg {
  node *ptr;
};

//the non-dsp thread writes this to the dsp thread
struct rt_msg {
  enum rt_msg_type type;
  union {
    struct add_msg add_msg;
    struct delete_msg delete_msg;
    struct connect_msg connect_msg;
    struct disconnect_msg disconnect_msg;
    struct control_msg control_msg;
    struct replace_patch_msg replace_patch_msg;
  };
};

//the dsp thread writes these to the non-dsp thread
struct non_rt_msg {
  enum non_rt_msg_type type;
  union {
    struct free_ptr_msg free_ptr_msg;
    struct free_node_msg free_node_msg;
  };
};

void free_outlet_connections(outlet out) {
  connection *ptr = out.connections;
  connection *prev;
  while(ptr) {
    prev = ptr;
    ptr = ptr->next;
    free(prev);
  }
}

void free_inlet_connections(inlet in) {
  connection *ptr = in.connections;
  connection *prev;
  while(ptr) {
    prev = ptr;
    ptr = ptr->next;
    free(prev);
  }
}

void destroy_inlets(node *n) {
  for(int i = 0; i < n->num_inlets; i++) {
    free_inlet_connections(n->inlets[i]);
    free(n->inlets[i].buf);
  }
  free(n->inlets);
}

void destroy_outlets(node *n) {
  for(int i = 0; i < n->num_outlets; i++) {
    free_outlet_connections(n->outlets[i]);
    free(n->outlets[i].buf);
  }
  free(n->outlets);
}

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
  unsigned int idx = n->id % TABLE_SIZE;

  //add to table
  node *head = p->table[idx];
  //if it's the first thing ever, just set head
  if(!head) {
    head = n;
    p->table[idx] = head;
    return;
  }

  //otherwise push it onto the stack
  n->next = head;
  head->prev = n;
  p->table[idx] = n;

  sort_patch(p);
}

void remove_node(patch *p, node *n) {
  if(n->prev) n->prev->next = n->next;
  if(n->next) n->next->prev = n->prev;
  unsigned int idx = n->id % TABLE_SIZE;
  if(p->table[idx] == n) p->table[idx] = n->next;

  sort_patch(p);
}

void free_node(node *n) {
  n->destroy(n);
  free(n);
}

void free_node_table(node_table table) {
  for(int i = 0; i < TABLE_SIZE; i++) {
    free_node_list(table[i]);
  }
  free(table);
}

node *init_node(const patch *p, int num_inlets, int num_outlets, int num_controls){
  assert(num_inlets >= 0);
  assert(num_outlets >= 0);
  assert(num_controls >= 0);
  node *n = malloc(sizeof(node));
  n->last_visited = -1;
  n->num_controls = num_controls;

  n->num_inlets = num_inlets;
  if(num_inlets > 0) n->inlets = malloc(sizeof(inlet) * num_inlets);

  n->num_outlets = num_outlets;
  if(num_outlets > 0) n->outlets = malloc(sizeof(outlet) * num_outlets);

  if(num_controls > 0) n->controls = malloc(sizeof(control) * num_controls);
  for(int i=0; i < num_controls; i++) {
    n->controls[i].set_control = NULL;
  }

  n->prev = NULL;
  n->next = NULL;
  return n;
}

node *new_node(const patch *p, const char *type) {
  //would love to not need this function but the alternative I can think of involves dynamic libs and sounds painful
  if(strcmp(type, "sin") == 0) {
    return new_sin_osc(p);
  }
  if(strcmp(type, "adc") == 0) {
    return new_adc(p);
  }
  if(strcmp(type, "buthp") == 0) {
    return new_buthp(p);
  }
  if(strcmp(type, "dac") == 0) {
    return new_dac(p);
  }
  if(strcmp(type, "gate_adc") == 0) {
    return new_gate_adc(p);
  }
  if(strcmp(type, "dist") == 0) {
    return new_dist(p);
  }
  if(strcmp(type, "delay") == 0) {
    return new_delay(p);
  }
  if(strcmp(type, "hip") == 0) {
    return new_hip(p);
  }
  if(strcmp(type, "mul") == 0) {
    return new_mul(p);
  }
  if(strcmp(type, "noise_gate") == 0) {
    return new_noise_gate(p);
  }
  if(strcmp(type, "looper") == 0) {
    return new_looper(p);
  }
  #ifdef BELA
  if(strcmp(type, "digiread") == 0) {
    return new_digiread(p);
  }
  #endif
  return NULL;
}

void init_inlet(const patch *p, node *n, int idx){
  assert(idx >= 0);
  assert(idx <= n->num_inlets);
  inlet *in = &n->inlets[idx];
  in->buf = calloc(p->audio_opts.buf_size, sizeof(float));
  in->buf_size = p->audio_opts.buf_size;
  in->num_connections = 0;
  in->connections = NULL;
}

void init_outlet(const patch *p, node *n, int idx){
  assert(idx >= 0);
  assert(idx <= n->num_outlets);
  outlet *out = &n->outlets[idx];
  out->buf = calloc(p->audio_opts.buf_size, sizeof(float));
  out->buf_size = p->audio_opts.buf_size;
  out->num_connections = 0;
  out->connections = NULL;
}

patch *new_patch(audio_options audio_opts) {
  patch *p = malloc(sizeof(patch));
  p->table = malloc(sizeof(node*) * TABLE_SIZE);
  for(int i = 0; i < TABLE_SIZE; i++) p->table[i] = NULL;
  p->next_id = 0;
  p->audio_opts = audio_opts;
  p->order.top = -1;
  p->hw_inlets = malloc(sizeof(inlet) * audio_opts.hw_out_channels);
  for(int i = 0; i < audio_opts.hw_out_channels; i++){
    p->hw_inlets[i].buf = calloc(audio_opts.buf_size, sizeof(float));
    p->hw_inlets[i].buf_size = audio_opts.buf_size;
    p->hw_inlets[i].num_connections = 0;
    p->hw_inlets[i].connections = NULL;
  }

  p->hw_outlets = malloc(sizeof(outlet) * audio_opts.hw_in_channels);
  for(int i = 0; i < audio_opts.hw_in_channels; i++){
    p->hw_outlets[i].buf = calloc(audio_opts.buf_size, sizeof(float));
    p->hw_outlets[i].buf_size = audio_opts.buf_size;
    p->hw_outlets[i].num_connections = 0;
    p->hw_outlets[i].connections = NULL;
  }

#ifdef BELA
  p->digital_outlets = malloc(sizeof(outlet) * audio_opts.digital_channels);
  for(int i = 0; i < audio_opts.digital_channels; i++){
    p->digital_outlets[i].buf = calloc(audio_opts.digital_frames, sizeof(float));
    p->digital_outlets[i].buf_size = audio_opts.digital_frames;
    p->digital_outlets[i].num_connections = 0;
    p->digital_outlets[i].connections = NULL;
  }
#endif
  return p;
}

blst_system *new_blst_system(audio_options audio_opts) {
  blst_system *bs = malloc(sizeof(blst_system));
  bs->audio_opts = audio_opts;
  bs->p = new_patch(audio_opts);
  tpipe_init(&bs->consumer_pipe, 1024 * 10);
  tpipe_init(&bs->producer_pipe, 1024 * 10);
  return bs;
}

/* int detect_cycles(const patch *p, int out_node_id, int in_node_id, int outlet_idx, int inlet_idx){ */
/*   //TODO: dfs thru, mark, if you've already marked something return -1 */
/*   return -1; */
/* } */

node *get_node(const patch *p, unsigned int id) {
  node *n = p->table[id % TABLE_SIZE];
  while(n) {
    if(n->id == id) return n;
    n = n->next;
  }
  return NULL;
}

void add_connection(patch *p, connection *out_conn, connection *in_conn) {
  //add connection to both in node and out node
  node *in_node = get_node(p, out_conn->node_id);
  node *out_node = get_node(p, in_conn->node_id);
  inlet *in = &in_node->inlets[out_conn->io_id];
  outlet *out = &out_node->outlets[in_conn->io_id];

  connection *in_ptr = in->connections;
  connection *out_ptr = out->connections;

  if(!in_ptr) { //it's the first connection, add it to head
    in->connections = in_conn;
    in->num_connections++;
  } else {
    connection *in_prev = in_ptr;
    while(in_ptr) {
      if(in_ptr->node_id == in_conn->node_id && in_ptr->io_id == in_conn->io_id){
        //if this is the case, we're already connected. return without connecting
        return;
      }
      in_prev = in_ptr;
      in_ptr = in_ptr->next;
    }
    in->num_connections++;
    in_prev->next = in_conn;
  }

  if(!out_ptr) {
    out->connections = out_conn;
    out->num_connections++;
  } else {
    connection *out_prev = out_ptr;
    while(out_ptr) {
      //don't need to check for dupes, should have caught it in the in_ptr case
      out_prev = out_ptr;
      out_ptr = out_ptr->next;
    }
    out->num_connections++;
    out_prev->next = out_conn;
  }

  sort_patch(p);
}

struct disconnect_pair {
  connection *in_conn;
  connection *out_conn;
};

struct disconnect_pair disconnect(const patch *p, int out_node_id, int outlet_idx, int in_node_id, int inlet_idx) {
  node *out_node = get_node(p, out_node_id);
  node *in_node = get_node(p, in_node_id);
  inlet *in = &in_node->inlets[inlet_idx];
  outlet *out = &out_node->outlets[outlet_idx];

  connection *out_ptr = out->connections;
  connection *out_prev = out_ptr;
  connection *in_ptr = in->connections;
  connection *in_prev = in_ptr;

  while(out_ptr) {
    if(out_ptr->node_id == in_node_id && out_ptr->io_id == inlet_idx){
      out->num_connections--;
      if(out_ptr == out_prev) {
        out->connections = out_ptr->next; //either the next connection, or NULL if there were none
      } else {
        out_prev->next = out_ptr->next;
      }
      break;
    }
    out_prev = out_ptr;
    out_ptr = out_ptr->next;
  }

  while(in_ptr) {
    if(in_ptr->node_id == out_node_id && in_ptr->io_id == outlet_idx) {
      in->num_connections--;
      if(in_ptr == in_prev) {
        in->connections = in_ptr->next; //either the next connection, or NULL if there were none
      } else {
        in_prev->next = in_ptr->next;
      }
      break;
    }
    in_prev = in_ptr;
    in_ptr = in_ptr->next;
  }

  struct disconnect_pair d = {in_ptr, out_ptr};
  return d;
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

void free_blst_system(blst_system *bs) {
  free_patch(bs->p);
  tpipe_free(&bs->consumer_pipe);
  tpipe_free(&bs->producer_pipe);
  free(bs);
}

void blst_connect(patch *p, unsigned int out_node_id, unsigned int outlet_id,
             unsigned int in_node_id, unsigned int inlet_id) {
  connection *out_conn = malloc(sizeof(connection));
  connection *in_conn = malloc(sizeof(connection));

  out_conn->next = NULL;
  out_conn->node_id = in_node_id;
  out_conn->io_id = inlet_id;

  in_conn->next = NULL;
  in_conn->node_id = out_node_id;
  in_conn->io_id = outlet_id;

  add_connection(p, out_conn, in_conn);
}

void dfs_visit(const patch *p, unsigned int generation, unsigned int node_id, struct int_stack *s) {
  node *n = get_node(p, node_id);
  n->last_visited = generation;
  for(int i = 0; i < n->num_outlets; i++) {
    connection *conn = n->outlets[i].connections;
    while(conn) {
      node *in = get_node(p, conn->node_id);
      if(in->last_visited != generation) {
        dfs_visit(p, generation, conn->node_id, s);
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

void zero_all_inlets(const patch *p) {
  struct int_stack s = p->order;
  for(int i = 0; i < p->audio_opts.hw_out_channels; i++) {
    float *b = p->hw_inlets[i].buf;
    for(int j = 0; j < p->hw_inlets[i].buf_size; j++) {
      b[j] = 0.f;
    }
  }
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

void set_control(node *n, int ctl_id, float val) {
  control *ctl = &n->controls[ctl_id];
  if (ctl->set_control) {
    /* printf("setting control the cool way!\n"); */
    ctl->set_control(ctl, val);
  } else {
    /* printf("setting control the uncool way!\n"); */
    ctl->val = val;
  }
}

void handle_rt_msg(blst_system *bs, struct rt_msg *msg) {
  patch *p = bs->p;
  switch(msg->type) {
    case ADD_NODE:
      add_node(p, msg->add_msg.node);
      sort_patch(p);
      break;
    case DELETE_NODE:
      {
        node *n = get_node(p, msg->delete_msg.node_id);
        //disconnect each inlet connection, disconnect each outlet connection
        for (int i = 0; i < n->num_inlets; i++) {
          connection *conn = n->inlets[i].connections;
          while (conn) {
            struct disconnect_pair d = disconnect(p, conn->node_id,
                                                     conn->io_id,
                                                     n->id,
                                                     i);
            struct free_ptr_msg f = {.ptr = d.in_conn};
            struct non_rt_msg msg = {.type = FREE_PTR, .free_ptr_msg = f};
            tpipe_write(&bs->producer_pipe, (char *)&msg, sizeof(struct non_rt_msg));
            conn = conn->next;
          }
        }
        for (int i = 0; i < n->num_outlets; i++) {
          connection *conn = n->outlets[i].connections;
          while (conn) {
            struct disconnect_pair d = disconnect(p, n->id, i,
                                                     conn->node_id,
                                                     conn->io_id);
            struct free_ptr_msg f = {.ptr = d.in_conn};
            struct non_rt_msg msg = {.type = FREE_PTR, .free_ptr_msg = f};
            tpipe_write(&bs->producer_pipe, (char *)&msg, sizeof(struct non_rt_msg));
            conn = conn->next;
          }
        }
        //now remove from node list
        remove_node(p, n);
        struct free_node_msg f = {.ptr = n};
        struct non_rt_msg msg = {.type = FREE_NODE, .free_node_msg = f};
        tpipe_write(&bs->producer_pipe, (char *)&msg, sizeof(struct non_rt_msg));
        break;
      }
    case CONNECT:
      add_connection(p, msg->connect_msg.out_conn,
                        msg->connect_msg.in_conn);
      break;
    case DISCONNECT:
      {
        struct disconnect_pair d = disconnect(p, msg->disconnect_msg.out_node_id,
                                                 msg->disconnect_msg.outlet_idx,
                                                 msg->disconnect_msg.in_node_id,
                                                 msg->disconnect_msg.inlet_idx);
        struct free_ptr_msg f = {.ptr = d.in_conn};
        struct non_rt_msg msg = {.type = FREE_PTR, .free_ptr_msg = f};
        tpipe_write(&bs->producer_pipe, (char *)&msg, sizeof(struct non_rt_msg));
        msg.free_ptr_msg.ptr = d.out_conn;
        tpipe_write(&bs->producer_pipe, (char *)&msg, sizeof(struct non_rt_msg));
        break;
      }
    case CONTROL:
      set_control(get_node(p, msg->control_msg.node_id), msg->control_msg.ctl_id, msg->control_msg.val);
      break;
    case REPLACE_PATCH: {
      *msg->replace_patch_msg.old_p = p;
      bs->p = msg->replace_patch_msg.new_p;
      *msg->replace_patch_msg.done = 1;
    break;
  }
  default:
    printf("handle_rt_msg: unrecognized msg_type\n");//TODO make some kind of exit handler
  }
}

void blst_process(blst_system *bs) {
  patch *p = bs->p;
  while(tpipe_hasData(&bs->consumer_pipe)){
    int len = 0;
    char *buf = NULL;
    buf = tpipe_getReadBuffer(&bs->consumer_pipe, &len);
    assert(len > 0);
    assert(buf != NULL);
    handle_rt_msg(bs, (struct rt_msg*) buf);
    tpipe_consume(&bs->consumer_pipe);
  }
  zero_all_inlets(p);
  int top = p->order.top;
  while(top >= 0) {
    node *n = get_node(p, p->order.stk[top]);
    n->process(n);

    //add outlet contents to connected inlets
    for(int i = 0; i < n->num_outlets; i++) {
      outlet out = n->outlets[i];
      connection *c = out.connections;
      while(c) {
        node *in_node = get_node(p, c->node_id);
        inlet in = in_node->inlets[c->io_id];
        for(int idx = 0; idx < out.buf_size; idx++) {
          in.buf[idx] += out.buf[idx];
        }
        c = c->next;
      }
    }
    top--;
  }
}

void no_op(struct node *self) {return;}

void handle_osc_message(blst_system *bs, tosc_message *osc) {
  char *address = tosc_getAddress(osc);
  if(strncmp(address, "/node/add", strlen("/node/add")) == 0) {
    const char *type = tosc_getNextString(osc);
    node *n = new_node(bs->p, type);
    struct add_msg body = {.node = n};
    struct rt_msg msg = {.type = ADD_NODE, .add_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *)&msg, sizeof(struct rt_msg));
  } else if(strncmp(address, "/node/connect", strlen("/node/connect")) == 0) {
    int32_t out_node_id = tosc_getNextInt32(osc);
    int32_t outlet_idx = tosc_getNextInt32(osc);
    int32_t in_node_id = tosc_getNextInt32(osc);
    int32_t inlet_idx = tosc_getNextInt32(osc);

    //TODO handle node not found or inlet not found, cycles
    connection *in_conn = malloc(sizeof(connection));
    connection *out_conn = malloc(sizeof(connection));
    out_conn->next = NULL;
    out_conn->node_id = in_node_id;
    out_conn->io_id = inlet_idx;
    in_conn->next = NULL;
    in_conn->node_id = out_node_id;
    in_conn->io_id = outlet_idx;
    struct connect_msg body = {.out_conn = out_conn, .in_conn = in_conn};
    struct rt_msg msg = {.type = CONNECT, .connect_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *)&msg, sizeof(struct rt_msg));
  } else if(strncmp(address, "/node/disconnect", strlen("/node/disconnect")) == 0) {
    int32_t out_node_id = tosc_getNextInt32(osc);
    int32_t outlet_idx = tosc_getNextInt32(osc);
    int32_t in_node_id = tosc_getNextInt32(osc);
    int32_t inlet_idx = tosc_getNextInt32(osc);
    struct disconnect_msg body = {.out_node_id = out_node_id, .outlet_idx = outlet_idx, in_node_id = in_node_id, inlet_idx = inlet_idx};
    struct rt_msg msg = {.type = DISCONNECT, .disconnect_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *)&msg, sizeof(struct rt_msg));
  } else if(strncmp(address, "/node/delete", strlen("/node/delete")) == 0) {
    int32_t node_id = tosc_getNextInt32(osc);
    struct delete_msg body = {.node_id = node_id};
    struct rt_msg msg = {.type = DELETE_NODE, .delete_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *) &msg, sizeof(struct rt_msg));
  } else if(strncmp(address, "/node/control", strlen("/node/control")) == 0) {
    int32_t node_id = tosc_getNextInt32(osc);
    int32_t ctl_id = tosc_getNextInt32(osc);
    float val = tosc_getNextFloat(osc);
    struct control_msg body = {.node_id = node_id, .ctl_id = ctl_id, .val = val};
    struct rt_msg msg = {.type = CONTROL, .control_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *) &msg, sizeof(struct rt_msg));
  } else if(strncmp(address, "/patch/free", strlen("/patch/free")) == 0) {
    patch *new_p = new_patch(bs->audio_opts);
    patch **old_p = malloc(sizeof(patch*));
    int *done = malloc(sizeof(int));
    *done = 0;
    struct replace_patch_msg body = {.done = done,
                                     .new_p = new_p,
                                     .old_p = old_p};
    struct rt_msg msg = {.type = REPLACE_PATCH,
                         .replace_patch_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *) &msg, sizeof(struct rt_msg));
    //busy wait for response - this may be a terrible idea, might really need to use atomic vars here but we'll see what happens
    while(! *done) {
      usleep(1000);
    }
    free_patch(*msg.replace_patch_msg.old_p);
    usleep(100);
  } else {
    printf("unexpected message: %s\n", address);
  }
}

void handle_non_rt_msg(blst_system *bs, struct non_rt_msg *msg) {
  switch(msg->type) {
    case FREE_PTR:
      printf("freeing pointer\n");
      free(msg->free_ptr_msg.ptr);
      break;
    case FREE_NODE:
      printf("freeing node\n");
      free_node(msg->free_node_msg.ptr);
      break;
    default:
      printf("handle_non_rt_msg: unrecognized msg_type\n");
  }
}

static volatile int keepRunning = 1;

void stop_osc_server() {
  keepRunning = 0;
}

void run_osc_server(blst_system *bs) {
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
    while(tpipe_hasData(&bs->producer_pipe)) {
      int len = 0;
      char *buf = NULL;
      buf = tpipe_getReadBuffer(&bs->producer_pipe, &len);
      assert(len > 0);
      assert(buf != NULL);
      handle_non_rt_msg(bs, (struct non_rt_msg*) buf);
      tpipe_consume(&bs->producer_pipe);
    }

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
            handle_osc_message(bs, &osc);
          }
        } else {
          tosc_message osc;
          tosc_parseMessage(&osc, buffer, len);
          handle_osc_message(bs, &osc);
        }
      }
    }
  }
}
