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
  blst_audio_options audio_opts;
  blst_patch *p;
  TinyPipe consumer_pipe, producer_pipe;
} blst_system;

blst_system *blst_new_system(blst_audio_options audio_opts);

void blst_free_system(blst_system *p);

enum blst_rt_msg_type {
  ADD_NODE,
  DELETE_NODE,
  CONNECT,
  DISCONNECT,
  CONTROL,
  REPLACE_PATCH,
};

enum blst_non_rt_msg_type {
 FREE_PTR,
 FREE_NODE,
};

struct blst_add_msg {
  blst_node *node;
};

struct blst_delete_msg {
  int32_t node_id;
};

struct blst_control_msg {
  int32_t node_id;
  int32_t ctl_id;
  float val;
};

struct blst_connect_msg {
  blst_connection *in_conn;
  blst_connection *out_conn;
};

struct blst_disconnect_msg {
  int32_t out_node_id;
  int32_t outlet_idx;
  int32_t in_node_id;
  int32_t inlet_idx;
};

struct blst_replace_patch_msg {
  int *done;
  blst_patch *new_p;
  blst_patch **old_p;
};

struct blst_free_ptr_msg {
  void *ptr;
};

struct blst_free_node_msg {
  blst_node *ptr;
};

struct blst_rt_msg {
  enum blst_rt_msg_type type;
  union {
    struct blst_add_msg add_msg;
    struct blst_delete_msg delete_msg;
    struct blst_connect_msg connect_msg;
    struct blst_disconnect_msg disconnect_msg;
    struct blst_control_msg control_msg;
    struct blst_replace_patch_msg replace_patch_msg;
  };
};

struct blst_non_rt_msg {
  enum blst_non_rt_msg_type type;
  union {
    struct blst_free_ptr_msg free_ptr_msg;
    struct blst_free_node_msg free_node_msg;
  };
};

blst_node *blst_new_node(const blst_patch *p, const char *type);
blst_system *blst_new_system(blst_audio_options audio_opts);
void blst_free_blst_system(blst_system *bs);
void blst_handle_rt_msg(blst_system *bs, struct blst_rt_msg *msg);
void blst_rt_process(blst_system *bs);

#endif
#ifdef __cplusplus
}
#endif
