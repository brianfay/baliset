#include "baliset_graph.h"
#include <string.h>
#include <stdio.h>
#include "sndfile.h"
#define SAMPLE_RATE 44100

int main() {
  audio_options a = {.buf_size = 1, .sample_rate = SAMPLE_RATE,
                     .hw_in_channels = 2, .hw_out_channels = 2};

  patch *p = new_patch(a);
  node *sin = new_sin_osc(p);
  node *flip = new_flip_flop(p);
  node *dac = new_dac(p);

  add_node(p, sin);
  add_node(p, flip);
  add_node(p, dac);

  blst_connect(p, sin->id, 0, flip->id, 0);
  blst_connect(p, flip->id, 0, dac->id, 1);
  blst_connect(p, flip->id, 1, dac->id, 1);

  SF_INFO info;
  memset(&info, 0, sizeof(SF_INFO));
  info.samplerate = SAMPLE_RATE;
  info.channels = 2;
  info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
  SNDFILE *sf = sf_open("flip_flop.wav", SFM_WRITE, &info);

  //seems to be interleaved

  for (int i = 0; i < SAMPLE_RATE * 3; i++) {
    blst_process(p);

    float b[2] = {p->hw_inlets[0].buf[0], p->hw_inlets[1].buf[0]};
    sf_write_float(sf, b, 2);
  }

  free_patch(p);
  sf_close(sf);
  return 0;
}
