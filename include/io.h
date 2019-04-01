#ifndef _IO_H_
#define _IO_H_

#include "portaudio.h"
#include <stdio.h>

#define SAMPLE_RATE (44100)
#define NUM_SECONDS 3

/* typedef struct */
/* { */
/*   float left_phase; */
/*   float right_phase; */
/*   float left_phase_inc; */
/*   float right_phase_inc; */
/* } */
/*   paTestData; */

void pa_run(PaStreamCallback *cb, void *data);

void pa_stop();


#endif
