all: trace.ascii trace.dot

%.png: %.dot
	dot -Tpng $< > $@

trace.dot: trace.ascii record.log
	../../graph/graph.py -a `./pick-attacker-pid.sh trace.ascii` -o $@ -l record.log

trace.ascii: record.log
	../../replay/dump $< > $@

repair: trace.ascii record.log
	cat /mnt/undofs/d/etc/shadow | grep root
	../../graph/graph.py -o repair.dot -v -r -a `./pick-attacker-pid.sh trace.ascii` -l record.log
	cat /mnt/undofs/d/etc/shadow | grep root

record.log: /tmp/record.log
	cp $< .

count: trace.dot
	cat trace.dot | grep -v '\->' | grep green | wc

clean:
	rm -f trace.dot trace.png trace.ascii record.log

.PHONY: clean repair