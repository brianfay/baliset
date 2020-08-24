#ifdef __cplusplus
extern "C" {
#endif
#include "baliset_rt.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <tinyosc.h>
#include <unistd.h>

#ifndef BALISET_OSC_SERVER
#define BALISET_OSC_SERVER


void blst_run_osc_server(blst_system *bs);
void blst_stop_osc_server();

#endif
#ifdef __cplusplus
}
#endif
