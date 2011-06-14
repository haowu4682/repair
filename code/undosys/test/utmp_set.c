#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utmp.h>

int
main(int ac, char **av)
{
    if (ac != 3) {
	fprintf(stderr, "Usage: %s login/logout utid\n", av[0]);
	exit(-1);
    }

    struct utmp ut;
    memset(&ut, 0, sizeof(ut));

    if (!strcmp(av[1], "login")) {
	ut.ut_type = USER_PROCESS;
    } else {
	ut.ut_type = DEAD_PROCESS;
    }

    ut.ut_pid = 1;
    snprintf(&ut.ut_line[0], sizeof(ut.ut_line), "pts/foo");
    snprintf(&ut.ut_user[0], sizeof(ut.ut_user), "blahuser");
    snprintf(&ut.ut_host[0], sizeof(ut.ut_host), "some.host.mit.edu");
    memcpy(&ut.ut_id[0], av[2], 4);

    pututline(&ut);
    if (ut.ut_type == USER_PROCESS)
        logwtmp(ut.ut_line, ut.ut_user, ut.ut_host);

    // must set the return value
    return 0;
}
