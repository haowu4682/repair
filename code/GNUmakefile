CPP             := g++
CFLAGS          := -Isrc/ -Isrc/common/

COMMONHDR       := $(wildcard src/common/*.h)
COMMONSRC       := $(wildcard src/common/*.cc)
COMMONOBJ       := $(patsubst bin/common/%.o, src/common/%.cc, $(COMMONSRC))
REPLAYHDR       := $(wildcard src/replay/*.h)
REPLAYSRC       := $(wildcard src/replay/*.cc)
REPLAYOBJ       := $(patsubst bin/replay/%.o, src/syscall/%.cc, $(REPLAYSRC))
SYSCALLHDR      := $(wildcard src/syscall/*.h)
SYSCALLSRC      := $(wildcard src/syscall/*.cc)
SYSCALLOBJ      := $(patsubst bin/syscall/%.o, src/syscall/%.cc, $(SYSCALLSRC))

REPLAYBIN       := bin/ProcessManager

all: bin

bin: $(REPLAYBIN)

$(REPLAYBIN): $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ)
	mkdir -p bin;
	$(CPP) $(CFLAGS) -o $@ $^

obj: $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ)

bin/common/%.o: src/common/%.cc src/common/*.h
	mkdir -p bin; mkdir -p bin/common;
	$(CPP) $(CFLAGS) -c $< -o $@

bin/replay/%.o: src/replay/%.cc src/replay/*.h src/syscall/*.h src/common/*.h
	mkdir -p bin; mkdir -p bin/replay;
	$(CPP) $(CFLAGS) -c $< -o $@

bin/syscall/%.o: src/syscall/%.cc src/syscall/*.h src/common/*.h
	mkdir -p bin; mkdir -p bin/replay;
	$(CPP) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(COMMONOBJ) $(REPLAYOBJ) $(SYSCALLOBJ)
