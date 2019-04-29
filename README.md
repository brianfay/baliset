# baliset
A realtime audio patching program, written in C.

Uses [TinyPipe](https://github.com/mhroth/tinypipe) and [TinyOSC](https://github.com/mhroth/tinyosc) by Martin Roth

Supported platforms are desktop via Portaudio, and [Bela](bela.io)

You will need a build directory:
```
mkdir build
```

To build on Desktop, run
```
make
```

To build on Bela, clone the project into `/root/baliset` or something and run
```
PROTOPATCH_ENV=bela make
```

To use the program, run
```
./build/baliset
```

If nothing works, run for the hills
