#include "protopatch.h"

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

void free_node_table(node_list_elem **node_table) {
  for(int i = 0; i < TABLE_SIZE; i++) {
    free_node_list(node_table[i]);
  }
  free(node_table);
}

inlet new_inlet(int buf_size, char *name) {
  inlet i;
  i.buf = calloc(buf_size, sizeof(float));
  i.buf_size = buf_size;
  i.num_connections = 0;
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
  g->node_table = malloc(sizeof(node_list_elem *) * TABLE_SIZE);
  for(int i = 0; i < TABLE_SIZE; i++) g->node_table[i] = NULL;
  g->num_nodes = 0;
  g->next_id = 0;
  g->audio_opts = a;
  g->num_hw_inlets = 2;
  g->hw_inlets = malloc(sizeof(inlet) * 2);
  g->hw_outlets = malloc(sizeof(outlet) * 2);
  g->hw_inlets[0] = new_inlet(a.buf_size, "left");
  g->hw_inlets[1] = new_inlet(a.buf_size, "right");
  g->num_hw_outlets = 2;
  g->hw_outlets[0] = new_outlet(a.buf_size, "left");
  g->hw_outlets[1] = new_outlet(a.buf_size, "right");
  return g;
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

void add_node(graph *g, node *n) {
  n->id = g->next_id++;
  g->num_nodes++;
  add_to_table(g->node_table, n);
}

node *get_node(graph *g, unsigned int id) {
  node_list_elem *e = search_for_elem(g->node_table[id % TABLE_SIZE], id);
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

void pp_connect(graph *g, unsigned int out_node_id, unsigned int outlet_id,
             unsigned int in_node_id, unsigned int inlet_id) {
  node *out = get_node(g, out_node_id);
  node *in = get_node(g, in_node_id);
  if (out && in && inlet_id < in->num_inlets 
      && outlet_id < out->num_outlets) {
    add_connection(&out->outlets[outlet_id], &in->inlets[inlet_id], in_node_id, inlet_id);
  }
}

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
    node_list_elem *ptr = g->node_table[i];
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

void zero_all_inlets(graph *g, struct int_stack s) {
  while(s.top >= 0) {
    node *n = get_node(g, s.stk[s.top]);
    for(int i = 0; i < n->num_inlets; i++) {
      inlet in = n->inlets[i];
      for(int j = 0; j < in.buf_size; j++) {
        in.buf[j] = 0.f;
      }
    }
    s.top--;
  }
}


void process_graph(graph *g, struct int_stack s) {
  zero_all_inlets(g, s);

  while(s.top >= 0) {
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
  //inlet output = g->hw_inlets[1];
  //for(int i = 0; i < g->audio_opts.buf_size; i++) {
  //  fwrite(&output.buf[i], 1, sizeof(float), stdout);
  //}
}
