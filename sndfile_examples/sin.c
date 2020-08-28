#include "baliset_graph.h"
#include <string.h>
#include <stdio.h>
#include "sndfile.h"
#define SAMPLE_RATE 44100

int main() {
  blst_audio_options a = {.buf_size = 1, .sample_rate = SAMPLE_RATE,
                          .hw_in_channels = 2, .hw_out_channels = 2};

  blst_patch *p = blst_new_patch(a);
  blst_node *sin = blst_new_sin_osc(p);
  blst_node *sin2 = blst_new_sin_osc(p);
  blst_node *dac = blst_new_dac(p);

  blst_add_node(p, sin);
  blst_add_node(p, sin2);
  blst_add_node(p, dac);

  blst_connect(p, sin->id, 0, dac->id, 0);
  blst_connect(p, sin2->id, 0, dac->id, 0);

  blst_connect(p, sin->id, 0, sin2->id, 0);
  /* blst_connect(p, sin->id, 0, dac->id, 1); */
  printf("connect err: %d\n", blst_connect(p, sin2->id, 0, sin->id, 0));

  /* printf("err: %d\n", blst_remove_node(p, sin)); */
  /* printf("err: %d\n", blst_remove_node(p, sin)); */

  /* blst_free_node(sin); */
  /* blst_free_node(&sin); */

  /* printf("err: %d\n", blst_free_node(&sin)); */
  /* printf("err: %d\n", blst_remove_node(p, sin)); */

  SF_INFO info;
  memset(&info, 0, sizeof(SF_INFO));
  info.samplerate = SAMPLE_RATE;
  info.channels = 2;
  info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
  info.frames = SAMPLE_RATE * 3;
  SNDFILE *sf = sf_open("sin.wav", SFM_WRITE, &info);

  //seems to be interleaved

  for (int i = 0; i < SAMPLE_RATE * 3; i++) {
    blst_process(p);

    float b[2] = {p->hw_inlets[0].buf[0], p->hw_inlets[1].buf[0]};
    sf_write_float(sf, b, 2);
  }

  blst_free_patch(p);
  sf_close(sf);
  return 0;
}
