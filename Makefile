#src = $(wildcard *.c)
#modules = $(wildcard modules/*.c)
#obj = $(wildcard target/*.o)

LDFLAGS = -lm -lportaudio
CFLAGS = -Wall -Iinclude -g

patch: src/patch.c io.o protopatch.o
	cc -o target/patch src/patch.c target/sin.o target/delay.o target/adc.o target/dac.o target/io.o target/protopatch.o $(CFLAGS) $(LDFLAGS)

io.o: src/io.c protopatch.o delay.o sin.o dac.o adc.o
	cc -c src/io.c -o target/io.o $(CFLAGS) $(LDFLAGS)

delay.o: nodes/delay.c protopatch.o
	cc -c nodes/delay.c -o target/delay.o $(CFLAGS) $(LDFLAGS)

adc.o: nodes/adc.c protopatch.o
	cc -c nodes/adc.c -o target/adc.o $(CFLAGS) $(LDFLAGS)

dac.o: nodes/dac.c protopatch.o
	cc -c nodes/dac.c -o target/dac.o $(CFLAGS) $(LDFLAGS)

sin.o: nodes/sin.c protopatch.o
	cc -c nodes/sin.c -o target/sin.o $(CFLAGS) $(LDFLAGS)

protopatch.o: src/protopatch.c
	cc -c src/protopatch.c -o target/protopatch.o $(CFLAGS) $(LDFLAGS)
#
#io.o: io.c
#	cc -c target/$@ protopatch.c $(CFLAGS) $(LDFLAGS)
.PHONY: clean

clean:
	rm target/*
