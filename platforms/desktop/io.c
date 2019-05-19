#include "io.h"
#include "portaudio.h"

void error();

PaStream *stream;

void pa_run(PaStreamCallback *cb, void *data)
{
  PaError err = Pa_Initialize();
  if(err != paNoError){
    error();
  }


  /* Open an audio I/O stream. */
  err = Pa_OpenDefaultStream(&stream,
                             2,
                             2,
                             paFloat32,
                             SAMPLE_RATE,
                             64,
                             cb,
                             data);
  if(err != paNoError) {error();}

  err = Pa_StartStream(stream);
  if(err != paNoError) {error();}
}

void pa_stop() {
  int err = Pa_StopStream( stream );
  if( err != paNoError ) {error();};

  err = Pa_CloseStream( stream );
  if( err != paNoError ) {error();}

  err = Pa_Terminate();
}

void error()
{
  printf("oh no, there was an error!\n");
}
