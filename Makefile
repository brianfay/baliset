BUILDDIR = build
TINYOSC = tinyosc
TINYPIPE = tinypipe
CFLAGS = -Wall -Iinclude -I$(TINYOSC) -I$(TINYPIPE) -lm -lsoundpipe -lsndfile

CC=cc
CXX=clang++

BALISET_SRC = $(wildcard src/*.c)
BALISET_TARGETS = $(BALISET_SRC:src/%.c=$(BUILDDIR)/%.o)
TINYOSC_TARGET = $(BUILDDIR)/tinyosc.o
TINYPIPE_TARGET = $(BUILDDIR)/tinypipe.o
NODE_SRCDIR = nodes
NODE_SRC = $(wildcard $(NODE_SRCDIR)/*.c)
ifeq ($(BALISET_ENV),bela)
	NODE_SRC += $(wildcard $(NODE_SRCDIR)/bela/*.c)
endif
NODE_TARGETS = $(NODE_SRC:nodes/%.c=$(BUILDDIR)/%.o)

ifeq ($(BALISET_ENV),bela)
	CFLAGS += -DBELA -I/root/Bela/include -L/root/Bela/lib -L/root/Bela/libraries -lbela -lbelaextra -O3 -march=armv7-a -mtune=cortex-a8 -mfloat-abi=hard -mfpu=neon -ftree-vectorize -ffast-math -DNDEBUG
else
	CFLAGS += -O3 -g -lportaudio
endif

ifeq ($(BALISET_ENV),bela)
$(BUILDDIR)/baliset: $(wildcard platforms/bela/*.cpp) $(BALISET_TARGETS) $(NODE_TARGETS) $(TINYOSC_TARGET) $(TINYPIPE_TARGET)
#I'm not sure if compiling the C files with a c compiiler and then the main program with a C++ compiler is a good idea
#but I guess I'll find out
	$(CXX) -o $@ $^ $(CFLAGS)
else
$(BUILDDIR)/baliset: $(wildcard platforms/desktop/*.c) $(BALISET_TARGETS) $(NODE_TARGETS) $(TINYOSC_TARGET) $(TINYPIPE_TARGET)
	$(CC) -o $@ $^ $(CFLAGS)
endif

$(BALISET_TARGETS): $(BALISET_SRC)
	$(CC) -c $< -o $@ $(CFLAGS)

$(NODE_TARGETS): $(BUILDDIR)/%.o : $(NODE_SRCDIR)/%.c include/baliset.h
	$(CC) -c $< -o $@ $(CFLAGS)

$(TINYOSC_TARGET): $(TINYOSC)/tinyosc.c
	$(CC) -c $< -o $@ -Werror -O0

$(TINYPIPE_TARGET): $(TINYPIPE)/tinypipe.c
	$(CC) -c $< -o $@ -Werror -O0

.PHONY: clean

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/baliset $(BUILDDIR)/bela/*
