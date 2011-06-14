APXS=apxs2 # ubuntu names apxs as apxs2

all: mod_retro.so

mod_retro.so: mod_retro.c
	$(APXS) -c $<

clean:
	rm -rf *.o *.lo *.slo *.la .libs

reload:
	./run.sh

install: mod_retro.c
	$(APXS) -c -a -i $<
	apachectl restart
