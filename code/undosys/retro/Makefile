all:
	@echo "make {clean|cscope|clean|TAGS|update}"

TAGS:
	ctags -e --exclude="bench" -R .

cscope:
	cscope-indexer -r

clean:
	for dir in trace ctl; do make -C $$dir clean ; done
	rm -f cscope.* TAGS

deploy:
	rsync -avz --exclude='.git' 	\
		--exclude='test/bench'  	\
		--exclude='test/retro.img' 	\
		. root@vm:~/retro

test: deploy
	ssh -X -t root@vm "./test; cd retro/repair; bash"

ssh: deploy
	ssh -X -t root@vm "cd retro/repair; bash"

profile: deploy
	ssh -X -t root@vm "cd retro/profile; ./profile.py; bash"

reset:
	VBoxManage controlvm ubuntu-10.04-64bit reset

update: TAGS cscope

.PHONY: all clean cscope ssh update deploy test TAGS
