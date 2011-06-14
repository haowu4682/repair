#ifndef _SYSARG_H
#define _SYSARG_H

enum {
    sysarg_end = 0,
    sysarg_int,
    sysarg_strnull,
    sysarg_argv,
    sysarg_buf_arglen,
    sysarg_buf_arglenp,
    sysarg_buf_fixlen,
    sysarg_ignore,
    
    // only one when entering or exiting
    sysarg_buf_arglen_when_exiting,
    sysarg_buf_arglen_when_entering,
    sysarg_argv_when_entering,
    sysarg_argv_when_exiting,
    
    sysarg_unknown,
};

struct sysarg_arg {
    int type;
    int lensz;
    int lenarg;
};

struct sysarg_call {
    const char *name;
    struct sysarg_arg args[7];
};

extern struct sysarg_call sysarg_calls[];

#endif
