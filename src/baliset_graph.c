#include "baliset_graph.h"

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
  //could we free all inlets/outlets here?
  n->destroy(n);
  free(n);
}

void free_node_table(node_table table) {
  for(int i = 0; i < TABLE_SIZE; i++) {
    free_node_list(table[i]);
  }
  free(table);
}

node *init_node(const patch *p, int num_inlets, int num_outlets, int num_controls) {
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

struct disconnect_pair blst_disconnect(const patch *p, int out_node_id, int outlet_idx, int in_node_id, int inlet_idx) {
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
    ctl->set_control(ctl, val);
  } else {
    ctl->val = val;
  }
}


void blst_process(const patch *p) {
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

void no_op(node *self) {
  return;
}
