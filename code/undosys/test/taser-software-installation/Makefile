all: trace.ascii trace.png

record.log:
	cp /tmp/record.log .

%.png: %.dot
	dot -Tpng $< > $@

trace.dot: trace.ascii record.log
	../../graph/graph.py -e -a `./pick-attacker-pid.sh trace.ascii` -o $@ -l record.log

trace.ascii: record.log
	../../replay/dump $< > $@

repair: trace.ascii record.log
	../../graph/graph.py -v -r -e -a `./pick-attacker-pid.sh trace.ascii` -l record.log

clean:
	rm -f trace.dot trace.png trace.ascii

.PHONY: clean repair