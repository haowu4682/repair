#ifndef _SYSARG_TYPES_H
#define _SYSARG_TYPES_H

/* Ugly.. */
#define stat xxstat
#define stat64 xxstat64
#include <asm/stat.h>
#undef stat
#undef stat64

#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/vfs.h>
#include <asm/posix_types.h>

#define old_uid_t __kernel_old_uid_t
#define old_gid_t __kernel_old_gid_t

#endif
