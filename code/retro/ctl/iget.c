#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

int
main(int ac, char **av)
{
    if (ac != 3) {
	fprintf(stderr, "Usage: %s dir inum\n", av[0]);
	fprintf(stderr, "Creates dir/.ino.inum\n");
	exit(-1);
    }

    int iget = open("/sys/kernel/debug/iget", O_RDONLY);
    if (iget < 0) {
	perror("opening iget ctl file");
	exit(-1);
    }

    int dir = open(av[1], O_RDONLY);
    if (dir < 0) {
	perror("opening dir");
	exit(-1);
    }

    long ino = atol(av[2]);
    if (ioctl(iget, dir, ino) < 0 && errno != -EEXIST) {
	perror("iget");
	exit(-1);
    }

    printf("inode %ld available as %s/.ino.%ld\n", ino, av[1], ino);
    return 0;
}
