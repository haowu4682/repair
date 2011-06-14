all: trace.ascii trace.dot

%.png: %.dot
	dot -Tpng $< > $@

trace.dot: trace.ascii record.log
	../../graph/graph.py -a `./pick-attacker-pid.sh trace.ascii` -o $@ -l record.log

trace.ascii: record.log
	../../replay/dump $< > $@

clean:
	rm -f trace.dot trace.png trace.ascii record.log

repair: trace.ascii record.log
	../../graph/graph.py -v -o repair.dot -r -a `./pick-attacker-pid.sh trace.ascii` -l record.log

count: trace.dot
	cat trace.dot | grep -v '\->' | grep green | wc

record.log:
	cp /tmp/record.log .

.PHONY: clean repair count