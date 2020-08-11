# baliset
An program that lets you build up an audio graph for live processing, by connecting nodes in a web interface. This may be used for example, to set up a virtual guitar pedalboard, controlled via a tablet.

Not really ready for public consumption, but you are welcome to check it out and do whatever you want with it.

Uses [TinyPipe](https://github.com/mhroth/tinypipe) and [TinyOSC](https://github.com/mhroth/tinyosc) by Martin Roth, which I copied into the repo, and relies on Paul Batchelor's [Soundpipe](https://github.com/PaulBatchelor/Soundpipe), which you will need to install.

Supported platforms are desktop via JACK, and [Bela](bela.io)

You will need a build directory:
```
mkdir build
```

To build on Desktop, run
```
make
```

To build on Bela, clone the project into `/root/baliset` or rsync it from another computer and run
```
export BALISET_ENV=bela
make clean
make
```

To start the audio server, run
```
./build/baliset
```

Then start the server for the web ui
```
cd ui/server
npm install # (if you haven't already done this)
node app.js
```

The clojurescript browser app has a [figwheel](https://github.com/bhauman/figwheel-main)-driven live-reload workflow. You can run this with
```
cd ui/client
clj -Afig
```

Or from emacs/cider, open `ui/client/src/baliset_ui/core.cljs`, run `cider-jack-in-cljs`, choose ``figwheel-main`, and type `:dev` when prompted

Alternatively, the browser app be built with clojurescript optimizations on:
```
cd ui/client
clj -Aprod
```

A dev build outputs to `ui/client/resources/public/cljs-out/dev-main.js`, while a prod build outputs to `ui/client/resources/public/cljs-out/prod-main.js`. 
Make sure the right one is included in `ui/client/resources/public/index.html`.

Then open your browser to localhost:3001 (the node ui server must be running).
