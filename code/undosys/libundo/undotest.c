#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include "undocall.h"

int
main(int ac, char **av)
{
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd < 0)
	perror("open");

    char *username = "root";
    undo_func_start("testmgr", "pwread", strlen(username), username);

    undo_mask_start(fd);

    struct passwd *pw = getpwnam(username);
    if (!pw)
	perror("getpwnam");
    undo_depend(fd, "root", "testmgr", 0, 1);

    undo_mask_end(fd);

    int retval = 1;
    undo_func_end(sizeof(retval), &retval);

    undo_func_start("testmgr", "fileread", 0, 0);
    char buf[1024];
    read(fd, &buf, sizeof(buf));
    undo_func_end(0, 0);

    close(fd);
}
