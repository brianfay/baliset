#include "baliset_graph.h"
#include <math.h>

void blst_free_outlet_connections(blst_outlet out) {
  blst_connection *ptr = out.connections;
  blst_connection *prev;
  while(ptr) {
    prev = ptr;
    ptr = ptr->next;
    free(prev);
  }
}

void blst_free_inlet_connections(blst_inlet in) {
  blst_connection *ptr = in.connections;
  blst_connection *prev;
  while(ptr) {
    prev = ptr;
    ptr = ptr->next;
    free(prev);
  }
}

void blst_destroy_inlets(blst_node *n) {
  if(n->num_inlets > 0) {
    for(int i = 0; i < n->num_inlets; i++) {
      blst_free_inlet_connections(n->inlets[i]);
      free(n->inlets[i].buf);
    }
    free(n->inlets);
  }
}

void blst_destroy_outlets(blst_node *n) {
  if(n->num_outlets > 0) {
    for(int i = 0; i < n->num_outlets; i++) {
      blst_free_outlet_connections(n->outlets[i]);
      free(n->outlets[i].buf);
    }
    free(n->outlets);
  }
}

void blst_free_node_list(blst_node *n) {
  blst_node *tmp;
  while (n) {
    tmp = n;
    n = n->next;
    if(tmp) {
      blst_free_node(tmp);
      free(tmp);
    }
  }
}

void blst_add_node(blst_patch *p, blst_node *n) {
  n->id = p->next_id++;
  unsigned int idx = n->id % TABLE_SIZE;

  //add to table
  blst_node *head = p->table[idx];
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

  blst_sort_patch(p);
}

void blst_remove_node(blst_patch *p, blst_node *n) {
  if(n->prev) n->prev->next = n->next;
  if(n->next) n->next->prev = n->prev;
  unsigned int idx = n->id % TABLE_SIZE;
  if(p->table[idx] == n) p->table[idx] = n->next;

  blst_sort_patch(p);
}

void blst_free_node(blst_node *n) {
  //could we free all inlets/outlets here?
  free(n->data);
  blst_destroy_inlets(n);
  blst_destroy_outlets(n);
  if(n->num_controls > 0) {
    free(n->controls);
  }
  if (n->destroy) n->destroy(n);
}

void blst_free_node_table(blst_node_table table) {
  for(int i = 0; i < TABLE_SIZE; i++) {
    blst_free_node_list(table[i]);
  }
  free(table);
}

blst_node *blst_init_node(const blst_patch *p, int num_inlets, int num_outlets, int num_controls) {
  assert(num_inlets >= 0);
  assert(num_outlets >= 0);
  assert(num_controls >= 0);
  blst_node *n = malloc(sizeof(blst_node));
  n->process = NULL;
  n->destroy = NULL;
  n->data = NULL;
  n->last_visited = -1;
  n->num_controls = num_controls;

  n->num_inlets = num_inlets;
  if(num_inlets > 0) n->inlets = malloc(sizeof(blst_inlet) * num_inlets);

  n->num_outlets = num_outlets;
  if(num_outlets > 0) n->outlets = malloc(sizeof(blst_outlet) * num_outlets);

  if(num_controls > 0) n->controls = malloc(sizeof(blst_control) * num_controls);
  for(int i=0; i < num_controls; i++) {
    n->controls[i].set_control = NULL;
  }

  n->prev = NULL;
  n->next = NULL;
  return n;
}

void blst_init_inlet(const blst_patch *p, blst_node *n, int idx){
  assert(idx >= 0);
  assert(idx <= n->num_inlets);
  blst_inlet *in = &n->inlets[idx];
  in->buf = calloc(p->audio_opts.buf_size, sizeof(float));
  in->buf_size = p->audio_opts.buf_size;
  in->num_connections = 0;
  in->connections = NULL;
}

void blst_init_outlet(const blst_patch *p, blst_node *n, int idx){
  assert(idx >= 0);
  assert(idx <= n->num_outlets);
  blst_outlet *out = &n->outlets[idx];
  out->buf = calloc(p->audio_opts.buf_size, sizeof(float));
  out->buf_size = p->audio_opts.buf_size;
  out->num_connections = 0;
  out->connections = NULL;
}

void blst_init_wavetables(blst_patch *p) {
  for(int i = 0; i < WAVETABLE_SIZE; i++) {
    p->wavetables.sine_table[i] = sinf((float)i / (float)WAVETABLE_SIZE * M_PI * 2.0);
  }
}

blst_patch *blst_new_patch(blst_audio_options audio_opts) {
  blst_patch *p = malloc(sizeof(blst_patch));
  blst_init_wavetables(p);
  p->table = malloc(sizeof(blst_node*) * TABLE_SIZE);
  for(int i = 0; i < TABLE_SIZE; i++) p->table[i] = NULL;
  p->next_id = 0;
  p->audio_opts = audio_opts;
  p->order.top = -1;
  p->hw_inlets = malloc(sizeof(blst_inlet) * audio_opts.hw_out_channels);
  for(int i = 0; i < audio_opts.hw_out_channels; i++){
    p->hw_inlets[i].buf = calloc(audio_opts.buf_size, sizeof(float));
    p->hw_inlets[i].buf_size = audio_opts.buf_size;
    p->hw_inlets[i].num_connections = 0;
    p->hw_inlets[i].connections = NULL;
  }

  p->hw_outlets = malloc(sizeof(blst_outlet) * audio_opts.hw_in_channels);
  for(int i = 0; i < audio_opts.hw_in_channels; i++){
    p->hw_outlets[i].buf = calloc(audio_opts.buf_size, sizeof(float));
    p->hw_outlets[i].buf_size = audio_opts.buf_size;
    p->hw_outlets[i].num_connections = 0;
    p->hw_outlets[i].connections = NULL;
  }

#ifdef BELA
  p->digital_outlets = malloc(sizeof(blst_outlet) * audio_opts.digital_channels);
  for(int i = 0; i < audio_opts.digital_channels; i++){
    p->digital_outlets[i].buf = calloc(audio_opts.digital_frames, sizeof(float));
    p->digital_outlets[i].buf_size = audio_opts.digital_frames;
    p->digital_outlets[i].num_connections = 0;
    p->digital_outlets[i].connections = NULL;
  }
#endif
  return p;
}

blst_node *blst_get_node(const blst_patch *p, unsigned int id) {
  blst_node *n = p->table[id % TABLE_SIZE];
  while(n) {
    if(n->id == id) return n;
    n = n->next;
  }
  return NULL;
}

void blst_add_connection(blst_patch *p, blst_connection *out_conn, blst_connection *in_conn) {
  //add connection to both in node and out node
  blst_node *in_node = blst_get_node(p, out_conn->node_id);
  blst_node *out_node = blst_get_node(p, in_conn->node_id);
  blst_inlet *in = &in_node->inlets[out_conn->io_id];
  blst_outlet *out = &out_node->outlets[in_conn->io_id];

  blst_connection *in_ptr = in->connections;
  blst_connection *out_ptr = out->connections;

  if(!in_ptr) { //it's the first connection, add it to head
    in->connections = in_conn;
    in->num_connections++;
  } else {
    blst_connection *in_prev = in_ptr;
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
    blst_connection *out_prev = out_ptr;
    while(out_ptr) {
      //don't need to check for dupes, should have caught it in the in_ptr case
      out_prev = out_ptr;
      out_ptr = out_ptr->next;
    }
    out->num_connections++;
    out_prev->next = out_conn;
  }

  blst_sort_patch(p);
}

struct blst_disconnect_pair blst_disconnect(const blst_patch *p, int out_node_id, int outlet_idx, int in_node_id, int inlet_idx) {
  blst_node *out_node = blst_get_node(p, out_node_id);
  blst_node *in_node = blst_get_node(p, in_node_id);
  blst_inlet *in = &in_node->inlets[inlet_idx];
  blst_outlet *out = &out_node->outlets[outlet_idx];

  blst_connection *out_ptr = out->connections;
  blst_connection *out_prev = out_ptr;
  blst_connection *in_ptr = in->connections;
  blst_connection *in_prev = in_ptr;

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

  struct blst_disconnect_pair d = {in_ptr, out_ptr};
  return d;
}

void blst_free_patch(blst_patch *p) {
  //free each node
  blst_free_node_table(p->table);
  for(int i = 0; i < p->audio_opts.hw_out_channels; i++) {
    free(p->hw_inlets[i].buf);
  }
  free(p->hw_inlets);

  for(int i = 0; i < p->audio_opts.hw_in_channels; i++) {
    free(p->hw_outlets[i].buf);
  }
  //TODO am I freeing digital outlets?
  free(p->hw_outlets);
  free(p);
}

void blst_connect(blst_patch *p, unsigned int out_node_id, unsigned int outlet_id,
                  unsigned int in_node_id, unsigned int inlet_id) {
  blst_connection *out_conn = malloc(sizeof(blst_connection));
  blst_connection *in_conn = malloc(sizeof(blst_connection));

  out_conn->next = NULL;
  out_conn->node_id = in_node_id;
  out_conn->io_id = inlet_id;

  in_conn->next = NULL;
  in_conn->node_id = out_node_id;
  in_conn->io_id = outlet_id;

  blst_add_connection(p, out_conn, in_conn);
}

void blst_dfs_visit(const blst_patch *p, unsigned int generation, unsigned int node_id, struct blst_int_stack *s) {
  blst_node *n = blst_get_node(p, node_id);
  n->last_visited = generation;
  for(int i = 0; i < n->num_outlets; i++) {
    blst_connection *conn = n->outlets[i].connections;
    while(conn) {
      blst_node *in = blst_get_node(p, conn->node_id);
      if(in->last_visited != generation) {
        blst_dfs_visit(p, generation, conn->node_id, s);
      }
      conn = conn->next;
    }
  }
  //push onto stack
  s->stk[s->top] = node_id;
  s->top++;
}

void blst_sort_patch(blst_patch *p) {
  static int generation = -1;
  generation++;
  struct blst_int_stack ordered_nodes = {.top = 0, .stk= {0}};
  for(int i = 0; i < TABLE_SIZE; i++) {
    blst_node *n = p->table[i];
    while(n) {
      if(n->last_visited != generation) {
        blst_dfs_visit(p, generation, n->id, &ordered_nodes);
      }
      n = n->next;
    }
  }
  ordered_nodes.top--;
  p->order = ordered_nodes;
}

void blst_zero_all_inlets(const blst_patch *p) {
  struct blst_int_stack s = p->order;
  for(int i = 0; i < p->audio_opts.hw_out_channels; i++) {
    float *b = p->hw_inlets[i].buf;
    for(int j = 0; j < p->hw_inlets[i].buf_size; j++) {
      b[j] = 0.f;
    }
  }
  while(s.top >= 0) {
    blst_node *n = blst_get_node(p, s.stk[s.top]);
    for(int i = 0; i < n->num_inlets; i++) {
      blst_inlet in = n->inlets[i];
      for(int j = 0; j < in.buf_size; j++) {
        in.buf[j] = 0.f;
      }
    }
    s.top--;
  }
}

void blst_set_control(blst_node *n, int ctl_id, float val) {
  blst_control *ctl = &n->controls[ctl_id];
  if (ctl->set_control) {
    ctl->set_control(ctl, val);
  } else {
    ctl->val = val;
  }
}


void blst_process(const blst_patch *p) {
  blst_zero_all_inlets(p);
  int top = p->order.top;
  while(top >= 0) {
    blst_node *n = blst_get_node(p, p->order.stk[top]);
    if(n->process) n->process(n);

    //add outlet contents to connected inlets
    for(int i = 0; i < n->num_outlets; i++) {
      blst_outlet out = n->outlets[i];
      blst_connection *c = out.connections;
      while(c) {
        blst_node *in_node = blst_get_node(p, c->node_id);
        blst_inlet in = in_node->inlets[c->io_id];
        for(int idx = 0; idx < out.buf_size; idx++) {
          in.buf[idx] += out.buf[idx];
        }
        c = c->next;
      }
    }
    top--;
  }
}
