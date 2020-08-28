#include "baliset_osc_server.h"

void blst_handle_osc_message(blst_system *bs, tosc_message *osc) {
  char *address = tosc_getAddress(osc);
  if(strncmp(address, "/node/add", strlen("/node/add")) == 0) {
    const char *type = tosc_getNextString(osc);
    blst_node *n = blst_new_node(bs->p, type);
    struct blst_add_msg body = {.node = n};
    struct blst_rt_msg msg = {.type = ADD_NODE, .add_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *)&msg, sizeof(struct blst_rt_msg));
  } else if(strncmp(address, "/node/connect", strlen("/node/connect")) == 0) {
    int32_t out_node_id = tosc_getNextInt32(osc);
    int32_t outlet_idx = tosc_getNextInt32(osc);
    int32_t in_node_id = tosc_getNextInt32(osc);
    int32_t inlet_idx = tosc_getNextInt32(osc);

    //TODO handle node not found or inlet not found, cycles
    blst_connection *in_conn = malloc(sizeof(blst_connection));
    blst_connection *out_conn = malloc(sizeof(blst_connection));
    out_conn->next = NULL;
    out_conn->node_id = in_node_id;
    out_conn->io_id = inlet_idx;
    in_conn->next = NULL;
    in_conn->node_id = out_node_id;
    in_conn->io_id = outlet_idx;
    struct blst_connect_msg body = {.out_conn = out_conn, .in_conn = in_conn};
    struct blst_rt_msg msg = {.type = CONNECT, .connect_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *)&msg, sizeof(struct blst_rt_msg));
  } else if(strncmp(address, "/node/disconnect", strlen("/node/disconnect")) == 0) {
    int32_t out_node_id = tosc_getNextInt32(osc);
    int32_t outlet_idx = tosc_getNextInt32(osc);
    int32_t in_node_id = tosc_getNextInt32(osc);
    int32_t inlet_idx = tosc_getNextInt32(osc);
    struct blst_disconnect_msg body = {.out_node_id = out_node_id, .outlet_idx = outlet_idx, in_node_id = in_node_id, inlet_idx = inlet_idx};
    struct blst_rt_msg msg = {.type = DISCONNECT, .disconnect_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *)&msg, sizeof(struct blst_rt_msg));
  } else if(strncmp(address, "/node/delete", strlen("/node/delete")) == 0) {
    int32_t node_id = tosc_getNextInt32(osc);
    struct blst_delete_msg body = {.node_id = node_id};
    struct blst_rt_msg msg = {.type = DELETE_NODE, .delete_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *) &msg, sizeof(struct blst_rt_msg));
  } else if(strncmp(address, "/node/control", strlen("/node/control")) == 0) {
    int32_t node_id = tosc_getNextInt32(osc);
    int32_t ctl_id = tosc_getNextInt32(osc);
    float val = tosc_getNextFloat(osc);
    struct blst_control_msg body = {.node_id = node_id, .ctl_id = ctl_id, .val = val};
    struct blst_rt_msg msg = {.type = CONTROL, .control_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *) &msg, sizeof(struct blst_rt_msg));
  } else if(strncmp(address, "/patch/free", strlen("/patch/free")) == 0) {
    blst_patch *new_p = blst_new_patch(bs->audio_opts);
    blst_patch **old_p = malloc(sizeof(blst_patch*));
    int *done = malloc(sizeof(int));
    *done = 0;
    struct blst_replace_patch_msg body = {.done = done,
                                          .new_p = new_p,
                                          .old_p = old_p};
    struct blst_rt_msg msg = {.type = REPLACE_PATCH,
                              .replace_patch_msg = body};
    tpipe_write(&bs->consumer_pipe, (char *) &msg, sizeof(struct blst_rt_msg));
    //busy wait for response - this may be a terrible idea, might really need to use atomic vars here but we'll see what happens
    while(! *done) {
      usleep(1000);
    }
    //UPDATE I did get a SIGABRT crash here but I think it was because I was not initializing the node data pointer to NULL
    blst_free_patch(*msg.replace_patch_msg.old_p);
    usleep(100);
  } else {
    printf("unexpected message: %s\n", address);
  }
}

void blst_handle_non_rt_msg(blst_system *bs, struct blst_non_rt_msg *msg) {
  switch(msg->type) {
  case FREE_PTR:
    printf("freeing pointer\n");
    free(msg->free_ptr_msg.ptr);
    break;
  case FREE_NODE:
    printf("freeing node\n");
    blst_free_node(&msg->free_node_msg.ptr);
    break;
  default:
    printf("handle_non_rt_msg: unrecognized msg_type\n");
  }
}

//does this need some kind of synchronization?
static volatile int keepRunning = 1;

void blst_stop_osc_server() {
  keepRunning = 0;
}

void blst_run_osc_server(blst_system *bs) {
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
      blst_handle_non_rt_msg(bs, (struct blst_non_rt_msg*) buf);
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
            blst_handle_osc_message(bs, &osc);
          }
        } else {
          tosc_message osc;
          tosc_parseMessage(&osc, buffer, len);
          blst_handle_osc_message(bs, &osc);
        }
      }
    }
  }
}
