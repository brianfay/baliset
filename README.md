A realtime audio patching program, written in C. Or at least... a prototype for one

Runs on Desktop via Portaudio, or on [Bela](bela.io)

You will need a build directory:
```mkdir build```

To build on Desktop, run
```make```

To build on Bela, clone the project into `/root/protopatch` or something and run
```PROTOPATCH_ENV=bela make```

To use the program, run
```./build/protopatch```

If nothing works, run for the hills
