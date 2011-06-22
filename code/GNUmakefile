CPP             := g++
CFLAGS          := -Isrc/
LDFLAGS         := -lrt

COMMONHDR       := $(wildcard src/common/*.h)
COMMONSRC       := $(wildcard src/common/*.cc)
COMMONOBJ       := $(patsubst src/common/%.cc, bin/common/%.o, $(COMMONSRC))
REPLAYHDR       := $(wildcard src/replay/*.h)
REPLAYSRC       := $(wildcard src/replay/*.cc)
REPLAYOBJ       := $(patsubst src/replay/%.cc, bin/replay/%.o, $(REPLAYSRC))
SYSCALLHDR      := $(wildcard src/syscall/*.h)
SYSCALLSRC      := $(wildcard src/syscall/*.cc)
SYSCALLOBJ      := $(patsubst src/syscall/%.cc, bin/syscall/%.o, $(SYSCALLSRC))

REPLAYBIN       := bin/ProcessManager

all: bin

bin: $(REPLAYBIN)

$(REPLAYBIN): $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ)
	mkdir -p bin;
	$(CPP) $(LDFLAGS) -o $@ $^

obj: $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ)

bin/common/%.o: src/common/%.cc src/common/*.h
	mkdir -p bin; mkdir -p bin/common;
	$(CPP) $(CFLAGS) -o $@ -c $<

bin/replay/%.o: src/replay/%.cc src/replay/*.h src/syscall/*.h src/common/*.h
	mkdir -p bin; mkdir -p bin/replay;
	$(CPP) $(CFLAGS) -o $@ -c $<

bin/syscall/%.o: src/syscall/%.cc src/syscall/*.h src/common/*.h src/syscall/trace_syscalls.inc
	mkdir -p bin; mkdir -p bin/syscall;
	$(CPP) $(CFLAGS) -o $@ -c $<

src/syscall/%.inc: src/syscall/%.py
	$< > $@

clean:
	rm -f $(REPLAYBIN) $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ) src/syscall/trace_syscalls.inc
