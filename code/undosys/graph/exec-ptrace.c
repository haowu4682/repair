#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm/unistd_64.h>

int
main(int ac, char **av)
{
    assert(ac > 2);

    if (strcmp(av[1], "/")) {
	if (chroot(av[1]))
	    err(1, "chroot %s", av[1]);
    }

    // XXX: setuid/gid?

    while (syscall(0xc0ffee) != 0)
	;

    execve(av[2], av + 3, environ);
    err(1, "execve %s", av[2]);
}
