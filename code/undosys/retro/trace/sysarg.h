#pragma once

struct sysarg;

/* This function pointer type doubles as an argument type and as a
 * function users call to _write_ the relevant type to the current
 * record.
 */
typedef void (*argtype_t)(long, const struct sysarg *);

struct sysarg {
	const char *name;
  argtype_t ty;
  int usage; /* specifies whether the sysarg should be written out with
              * the entry record (usage == 0) or the exit record (usage == 1)
              */
	long aux;
	long ret;
};

#define SYSARG_(type) void sysarg_##type(long v, const struct sysarg *arg)

SYSARG_(void);
SYSARG_(sint);
SYSARG_(uint);
SYSARG_(sint32);
SYSARG_(uint32);
SYSARG_(intp);
SYSARG_(pid_t);
SYSARG_(buf);
SYSARG_(sha1);
SYSARG_(string);
SYSARG_(strings);
SYSARG_(iovec);
SYSARG_(fd);
SYSARG_(fd2);
SYSARG_(name);
SYSARG_(path);
SYSARG_(path_at);
SYSARG_(rpath);
SYSARG_(rpath_at);
SYSARG_(buf_det);
SYSARG_(struct);
SYSARG_(psize_t);
SYSARG_(msghdr);

#define sysarg_dirfd	sysarg_fd

SYSARG_(execve);
