#ifndef BALISET_RT
#define BALISET_RT
#ifdef __cplusplus
extern "C" {
#endif
#include "tinypipe.h"
#include <string.h>
#include "baliset_graph.h"

//top level app state
typedef struct blst_system {
  audio_options audio_opts;
  patch *p;
  TinyPipe consumer_pipe, producer_pipe;
} blst_system;

blst_system *new_blst_system(audio_options audio_opts);

void free_blst_system(blst_system *p);

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

struct non_rt_msg {
  enum non_rt_msg_type type;
  union {
    struct free_ptr_msg free_ptr_msg;
    struct free_node_msg free_node_msg;
  };
};

node *new_node(const patch *p, const char *type);
blst_system *new_blst_system(audio_options audio_opts);
void free_blst_system(blst_system *bs);
void handle_rt_msg(blst_system *bs, struct rt_msg *msg);
void blst_rt_process(blst_system *bs);

#endif
#ifdef __cplusplus
}
#endif
