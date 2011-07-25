CPP             := g++
CXXFLAGS        := -Isrc/ -g
LDFLAGS         := -lrt -lssl -g

COMMONHDR       := $(wildcard src/common/*.h)
COMMONSRC       := $(wildcard src/common/*.cc)
COMMONOBJ       := $(patsubst src/common/%.cc, bin/common/%.o, $(COMMONSRC))
REPLAYHDR       := $(wildcard src/replay/*.h)
REPLAYSRC       := $(wildcard src/replay/*.cc)
REPLAYOBJ       := $(patsubst src/replay/%.cc, bin/replay/%.o, $(REPLAYSRC))
SYSCALLHDR      := $(wildcard src/syscall/*.h)
SYSCALLSRC      := $(wildcard src/syscall/*.cc)
SYSCALLOBJ      := $(patsubst src/syscall/%.cc, bin/syscall/%.o, $(SYSCALLSRC))

REPLAYBIN       := bin/SystemReplay

all: bin

bin: $(REPLAYBIN)

$(REPLAYBIN): $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ)
	mkdir -p bin;
	$(CPP) $(LDFLAGS) -o $@ $^

obj: $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ)

bin/common/%.o: src/common/%.cc src/common/*.h
	mkdir -p bin; mkdir -p bin/common;
	$(CPP) $(CXXFLAGS) -o $@ -c $<

bin/replay/%.o: src/replay/%.cc src/replay/*.h src/syscall/*.h src/common/*.h
	mkdir -p bin; mkdir -p bin/replay;
	$(CPP) $(CXXFLAGS) -o $@ -c $<

bin/syscall/%.o: src/syscall/%.cc src/syscall/*.h src/common/*.h src/replay/*.h src/syscall/trace_syscalls.inc src/syscall/trace_syscalls_exec.inc
	mkdir -p bin; mkdir -p bin/syscall;
	$(CPP) $(CXXFLAGS) -o $@ -c $<

src/syscall/trace_syscalls.inc: src/syscall/trace_syscalls.py
	$< > $@

src/syscall/trace_syscalls_exec.inc: src/syscall/trace_syscalls_exec.py
	$< > $@

retro: retro/*
	cd retro; make -C ctl; make -C trace

clean:
	cd retro; make clean
	rm -f $(REPLAYBIN) $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ) src/syscall/trace_syscalls.inc src/syscall/trace_syscalls_exec.inc

