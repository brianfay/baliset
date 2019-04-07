#include "protopatch.h"
#include "string.h"

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
node_list_elem *new_node_list_elem() {
  node_list_elem *elem = malloc(sizeof(node_list_elem));
  elem->prev = NULL;
  elem->next = NULL;
  elem->node = NULL;
  return elem;
}

node_list_elem *search_for_elem(node_list_elem *head, unsigned int node_id) {
  node_list_elem *ptr = head;
  while(ptr && ptr->node) {
    if(ptr->node->id == node_id) return ptr;
    ptr = ptr->next;
  }
  return NULL;
}

void free_node_list(node_list_elem *ptr) {
  node_list_elem *tmp;
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

void add_to_table(node_list_elem **nt, node *n) {
  unsigned int idx = n->id % TABLE_SIZE;
  node_list_elem *head = nt[idx];
  //if it's the first thing ever, just set head
  if(!head) {
    head = new_node_list_elem();
    head->node = n;
    nt[idx] = head;
    return;
  }

  //otherwise allocate new pointer and push it onto head of list
  node_list_elem *elem = new_node_list_elem();
  elem->node = n;
  elem->next = head;
  head->prev = elem;
//uhhh
  nt[idx] = elem;
}

void free_node_table(node_table table) {
  for(int i = 0; i < TABLE_SIZE; i++) {
    free_node_list(table[i]);
  }
  free(table);
}

inlet new_inlet(int buf_size, char *name, float default_val) {
  inlet i;
  i.buf = calloc(buf_size, sizeof(float));
  i.buf_size = buf_size;
  i.num_connections = 0;
  i.val = default_val;
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

patch *new_patch() {
  audio_options a = {.buf_size = 64, .sample_rate = 44100,
                     .hw_in_channels = 2, .hw_out_channels = 2};
  patch *p = malloc(sizeof(patch));
  p->table = malloc(sizeof(node_list_elem *) * TABLE_SIZE);
  for(int i = 0; i < TABLE_SIZE; i++) p->table[i] = NULL;
  p->num_nodes = 0;
  p->next_id = 0;
  p->audio_opts = a;
  p->num_hw_inlets = 2;
  p->hw_inlets = malloc(sizeof(inlet) * 2);
  p->hw_outlets = malloc(sizeof(outlet) * 2);
  p->hw_inlets[0] = new_inlet(a.buf_size, "left", 0.0);
  p->hw_inlets[1] = new_inlet(a.buf_size, "right", 0.0);
  p->num_hw_outlets = 2;
  p->hw_outlets[0] = new_outlet(a.buf_size, "left");
  p->hw_outlets[1] = new_outlet(a.buf_size, "right");
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

void add_node(patch *p, node *n) {
  n->id = p->next_id++;
  p->num_nodes++;
  add_to_table(p->table, n);
}

node *get_node(const patch *p, unsigned int id) {
  node_list_elem *e = search_for_elem(p->table[id % TABLE_SIZE], id);
  if (e) return e->node;
  return NULL;
}

void free_patch(patch *p) {
  //free each node
  free_node_table(p->table);
  for(int i = 0; i < p->num_hw_inlets; i++) {
    free(p->hw_inlets[i].buf);
  }
  free(p->hw_inlets);

  for(int i = 0; i < p->num_hw_outlets; i++) {
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
    node_list_elem *ptr = p->table[i];
    while(ptr && ptr->node) {
      if(ptr->node->last_visited != generation) {
        dfs_visit(p, generation, ptr->node->id, &ordered_nodes);
      }
      ptr = ptr->next;
    }
  }
  printf("ordered nodes.top: %d\n", ordered_nodes.top);
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

void process_patch(patch *p) {
  zero_all_inlets(p, p->order);
  int top = p->order.top;
  while(top >= 0) {
    node *n = get_node(p, p->order.stk[top]);
    n->process(n);

    //add outlet contents to connected inlets
    for(int i = 0; i < n->num_outlets; i++) {
      outlet o = n->outlets[i];
      connection *c = o.connections;
      while(c) {
        node *in_node = get_node(p, c->in_node_id);
        inlet in = in_node->inlets[c->inlet_id];
        for(int j = 0; j < p->audio_opts.buf_size; j++) {
          in.buf[j] += o.buf[j];
        }
        c = c->next;
      }
    }
    top--;
  }
  //inlet output = g->hw_inlets[1];
  //for(int i = 0; i < g->audio_opts.buf_size; i++) {
  //  fwrite(&output.buf[i], 1, sizeof(float), stdout);
  //}
}

void set_control(node *n, char *ctl_name, float val){
  for (int i = 0; i < n->num_inlets; i++){
    inlet *in = &n->inlets[i];
    if(strcmp(in->name, ctl_name) == 0) {
      in->val = val;
      return;
    }
  }
}

void no_op(struct node *self) {return;}
