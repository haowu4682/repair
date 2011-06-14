build:
	-test ! -e /tmp/apache2.pid || (kill -9 `cat /tmp/apache2.pid` ; true)
	cp sapi_apache2.c php5-5.3.3/sapi/apache2handler
	(cd php5-5/apache2-build; make)

run:
	while true; do ./run.sh ; sleep 0.1 ; done

rerun:
	while true; do RETRO_RERUN=/tmp/test ./run.sh ; sleep 0.1 ; done

setup:
	@echo "Please revise this sequence if you are trying to install :)"
	sudo apt-get install apache2
	sudo apt-get build-dep apache2
	sudo apt-get install php5-dev
	sudo apt-get build-dep php5

	@echo "Getting php source"
	rm -rf php5-5*
	apt-get source php5
	rm php5_*
	ln -s php5-5* php5-5
	(cd php5-5; debian/rules build)

.PHONY: build run setup
