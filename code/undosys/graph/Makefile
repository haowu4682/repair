## copy /tmp/record.log to record.log after recording

all: trace.ascii trace.png trace-collapse.png traceback.png exec-ptrace

record.py: ../replay/record.x rpcgen.py
	./rpcgen.py ../replay/record.x > $@

%.png: %.dot
	dot -Tpng $< > $@

trace.dot: trace.ascii record.log graph.py record.py
	./graph.py -e -a `./pick-attacker-pid.sh trace.ascii` -o $@ -l record.log

%-collapse.dot: %.dot
	cat $< | sort | ./edge-collapse.pl > $@

traceback.dot: trace.ascii record.log graph.py record.py
	./graph.py -e -b `./pick-line.sh trace.ascii` -o $@ -l record.log

trace.ascii: record.log
	../replay/dump $< > $@

clean:
	rm -f record.py trace.ascii trace.dot trace.png traceback.dot traceback.png

repair: trace.ascii record.log graph.py record.py exec-ptrace
	./graph.py -v -r -e -a `./pick-attacker-pid.sh trace.ascii` -l record.log

pdb: trace.ascii record.log graph.py record.py exec-ptrace
	python -m pdb ./graph.py -v -r -e -a `./pick-attacker-pid.sh trace.ascii` -l record.log

exec-ptrace: exec-ptrace.o
	$(CC) $< -o $@

exec-ptrace.o: exec-ptrace.c
	$(CC) $< -c -o $@ -std=c99 -D_GNU_SOURCE=1 -Wall -Werror

.PHONY: clean repair

