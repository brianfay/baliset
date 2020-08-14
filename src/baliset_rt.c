#include "baliset_rt.h"

node *new_node(const patch *p, const char *type) {
  //would love to not need this function but the alternative I can think of involves dynamic libs and sounds painful
  if(strcmp(type, "adc") == 0) {
    return new_adc(p);
  }
  if(strcmp(type, "buthp") == 0) {
    return new_buthp(p);
  }
  if(strcmp(type, "sin") == 0) {
    return new_sin_osc(p);
  }
  if(strcmp(type, "dac") == 0) {
    return new_dac(p);
  }
  if(strcmp(type, "delay") == 0) {
    return new_delay(p);
  }
  if(strcmp(type, "dist") == 0) {
    return new_dist(p);
  }
#ifdef BELA
  if(strcmp(type, "digiread") == 0) {
    return new_digiread(p);
  }
#endif
  if(strcmp(type, "flip_flop") == 0) {
    return new_flip_flop(p);
  }
  if(strcmp(type, "float") == 0) {
    return new_float(p);
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
  return NULL;
}

blst_system *new_blst_system(audio_options audio_opts) {
  blst_system *bs = malloc(sizeof(blst_system));
  bs->audio_opts = audio_opts;
  bs->p = new_patch(audio_opts);
  tpipe_init(&bs->consumer_pipe, 1024 * 10);
  tpipe_init(&bs->producer_pipe, 1024 * 10);
  return bs;
}

void free_blst_system(blst_system *bs) {
  free_patch(bs->p);
  tpipe_free(&bs->consumer_pipe);
  tpipe_free(&bs->producer_pipe);
  free(bs);
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
            struct disconnect_pair d = blst_disconnect(p, conn->node_id,
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
            struct disconnect_pair d = blst_disconnect(p, n->id, i,
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
        struct disconnect_pair d = blst_disconnect(p, msg->disconnect_msg.out_node_id,
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

void blst_rt_process(blst_system *bs) {
  const patch *p = bs->p;
  while(tpipe_hasData(&bs->consumer_pipe)){
    int len = 0;
    char *buf = NULL;
    buf = tpipe_getReadBuffer(&bs->consumer_pipe, &len);
    assert(len > 0);
    assert(buf != NULL);
    handle_rt_msg(bs, (struct rt_msg*) buf);
    tpipe_consume(&bs->consumer_pipe);
  }
  blst_process(p);
}
