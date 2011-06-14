all:
	cd ../syslogs && make rmmod
	sudo ./setup.sh
	sudo chmod -R a+w /mnt/undofs/d
	sudo chmod -R a+rw /mnt/undofs/snap
	cp record.log ../graph

test:
	cd ../graph && make

dump:
	../replay/dump2 record.log

repair:
	cd ../graph && make repair

clean:
	rm -f *~ *.txt
