#include "baliset_osc_server.h"
#include <jack/jack.h>
/* #include "string.h" */
/* #include "tinypipe.h" */

jack_port_t *output_left, *output_right, *input_left, *input_right;
jack_client_t *client;

int process(jack_nframes_t nframes, void *arg) {
  blst_system *bs = (blst_system*) arg;

  jack_default_audio_sample_t *inL, *inR, *outL, *outR;

  inL  = (jack_default_audio_sample_t*)jack_port_get_buffer(input_left, nframes);
  inR  = (jack_default_audio_sample_t*)jack_port_get_buffer(input_right, nframes);
  outL = (jack_default_audio_sample_t*)jack_port_get_buffer(output_left, nframes);
  outR = (jack_default_audio_sample_t*)jack_port_get_buffer(output_right, nframes);

  for(int i=0; i<nframes; i++) {
    bs->p->hw_outlets[0].buf[i] = inL[i];
    bs->p->hw_outlets[1].buf[i] = inR[i];
  }

  blst_rt_process(bs);

  for(int i=0; i<nframes; i++) {
    outL[i] = bs->p->hw_inlets[0].buf[i];
    outR[i] = bs->p->hw_inlets[1].buf[i];
  }
  return 0;
}

void jack_shutdown(void *arg) {
  fprintf(stderr, "jack server stopped or decided to disconnect baliset\n");
  fprintf(stderr, "shutting down server\n");
  exit(1);
}

int main() {
  srand(0);
  const char **in_ports;
  const char **out_ports;
  jack_options_t options = JackNullOption;
  jack_status_t status;

  client = jack_client_open("baliset", options, &status, NULL);

  if(client == NULL) {
    fprintf(stderr, "jack_client_open() failed, "
            "status = 0x%2.0x\n", status);
    if(status & JackServerFailed) {
      fprintf(stderr, "Unable to connect to JACK server\n");
    }
    exit(1);
  }

  if(status & JackNameNotUnique) {
    fprintf(stderr, "unique name '%s' assigned\n", jack_get_client_name(client));
  }

  blst_audio_options a = {.buf_size = jack_get_buffer_size(client), .sample_rate = jack_get_sample_rate(client),
                     .hw_in_channels = 2, .hw_out_channels = 2};
  blst_system *bs = blst_new_system(a);

  jack_set_process_callback(client, process, bs);
  jack_on_shutdown(client, jack_shutdown, 0);

  output_left = jack_port_register(client, "out_left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  output_right = jack_port_register(client, "out_right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  input_left = jack_port_register(client, "in_left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  input_right = jack_port_register(client, "in_right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

  if(jack_activate(client)){
    fprintf(stderr, "cannot activate client");
    exit(1);
  }

  //physica output ports should connect to baliset input ports, and vice versa
  out_ports = jack_get_ports(client, NULL, NULL,
                            JackPortIsPhysical|JackPortIsOutput);

  in_ports = jack_get_ports(client, NULL, NULL,
                            JackPortIsPhysical|JackPortIsInput);

  if(out_ports == NULL) {
    fprintf(stderr, "no physical recording ports\n");
  } else {
    if(jack_connect(client, out_ports[0], jack_port_name(input_left))) {
      fprintf(stderr, "could not connect left recording point to left baliset hw input port\n");
    }
    if(jack_connect(client, out_ports[1], jack_port_name(input_right))) {
      fprintf(stderr, "could not connect right recording point to left baliset hw input port\n");
    }
  }

  if(in_ports == NULL) {
    fprintf(stderr, "no physical playback ports\n");
  } else {
    if(jack_connect(client, jack_port_name(output_left), in_ports[0])) {
      fprintf(stderr, "could not connect left baliset hw output to left jack playback output\n");
    }
    if(jack_connect(client, jack_port_name(output_right), in_ports[1])) {
      fprintf(stderr, "could not connect right baliset hw output to left jack playback output\n");
    }
  }

  blst_run_osc_server(bs); //this blocks

  blst_free_system(bs);
}

//TODO: maybe handle jack server shutdown by waiting for new server to start up
//handle other callbacks, like jack_set_buffer_size_callback, jack_set_sample_rate_callback
//handle sessions, play nicely with LADISH or whatever the current way to do that is
//handle scenarios that aren't just stereo in/out
