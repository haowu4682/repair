all: trace.ascii trace.png

root:
	rm -f pick-attacker-pid.sh
	ln -s pick-attacker-pid-root.sh pick-attacker-pid.sh

A:
	rm -f pick-attacker-pid.sh
	ln -s pick-attacker-pid-A.sh pick-attacker-pid.sh

B:
	rm -f pick-attacker-pid.sh
	ln -s pick-attacker-pid-B.sh pick-attacker-pid.sh

record.log:
	cp /tmp/record.log .

%.png: %.dot
	dot -Tpng $< > $@

trace.dot: trace.ascii record.log pick-attacker-pid.sh
	../../graph/graph.py -e -a `./pick-attacker-pid.sh trace.ascii` -o $@ -l record.log

trace.ascii: record.log
	../../replay/dump $< > $@

repair: trace.ascii record.log
	cd ../../graph; ./graph.py -v -r -e -a `../test/taser-unhappy-student/pick-attacker-pid.sh ../test/taser-unhappy-student/trace.ascii` -l ../test/taser-unhappy-student/record.log

clean:
	rm -f trace.dot trace.png trace.ascii

.PHONY: clean repair A B root