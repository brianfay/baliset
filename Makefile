BUILDDIR = build
CFLAGS = -Wall -Iinclude -lm

CC=cc
CXX=clang++

PROTOPATCH_SRC = $(wildcard src/*.c)
PROTOPATCH_TARGETS = $(PROTOPATCH_SRC:src/%.c=$(BUILDDIR)/%.o)
NODE_SRCDIR = nodes
NODE_SRC = $(wildcard $(NODE_SRCDIR)/*.c)
ifeq ($(PROTOPATCH_ENV),bela)
	NODE_SRC += $(wildcard $(NODE_SRCDIR/bela/*.c))
endif
NODE_TARGETS = $(NODE_SRC:nodes/%.c=$(BUILDDIR)/%.o)

ifeq ($(PROTOPATCH_ENV),bela)
	CFLAGS += -DBELA -I/root/Bela/include -L/root/Bela/lib -lbela -O3 -march=armv7-a -mtune=cortex-a8 -mfloat-abi=hard -mfpu=neon -ftree-vectorize -ffast-math -DNDEBUG
else
	CFLAGS += -O3 -g -lportaudio
endif

ifeq ($(PROTOPATCH_ENV),bela)
build/protopatch: $(wildcard backends/bela/*.cpp) $(PROTOPATCH_TARGETS) $(NODE_TARGETS)
#I'm not sure if compiling the C files with a c compiiler and then the main program with a C++ compiler is a good idea
#but I guess I'll find out
	$(CXX) -o $@ $^ $(CFLAGS)
else
build/protopatch: $(wildcard backends/desktop/*.c) $(PROTOPATCH_TARGETS) $(NODE_TARGETS)
	$(CC) -o $@ $^ $(CFLAGS)
endif

$(PROTOPATCH_TARGETS): $(PROTOPATCH_SRC)
	$(CC) -c $< -o $@ $(CFLAGS)

$(NODE_TARGETS): $(BUILDDIR)/%.o : $(NODE_SRCDIR)/%.c include/protopatch.h
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean

clean:
	rm $(BUILDDIR)/*
