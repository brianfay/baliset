cc -I../include -lsndfile -lm ../build/baliset_graph.o ../build/sin.o ../build/dac.o sin.c  -o sin
cc -I../include -lsndfile -lm ../build/baliset_graph.o ../build/sin.o ../build/dac.o ../build/flip_flop.o flip_flop.c  -o flip_flop
