#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pwd.h>
#include <utmp.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include "undocall.h"

static struct passwd * (*lower_getpwnam) (const char *);
static struct passwd * (*lower_getpwuid) (uid_t);
static int (*lower_getpwnam_r) (const char *, struct passwd *,
				char *, size_t, struct passwd **);
static int (*lower_getpwuid_r) (uid_t, struct passwd *,
				char *, size_t, struct passwd **);
static struct utmp * (*lower_pututline) (const struct utmp *);
static struct utmp * (*lower_getutid) (const struct utmp *);
static int (*lower_getutid_r) (const struct utmp *, struct utmp *,
			       struct utmp **);

static int
resolve_symbol(const char *name, void **func)
{
    if (*func == 0)
	*func = dlsym(RTLD_NEXT, name);

    if (*func)
	return 0;

    fprintf(stderr, "unable to resolve symbol %s", name);
    return -1;
}

#define ENSURE_VALID(func, reterr)					\
    do {								\
	if (resolve_symbol(#func, (void **) &lower_##func) < 0)		\
	    return reterr;						\
    } while (0)

/* 
 * Password files have two kinds of sub-objects: name-based and uid-based.
 * Accessed by name or by UID just depend on the name or UID sub-object.
 * Updates should update both the name and UID sub-objects for the new name
 * and new UID.
 */

struct passwd *
getpwnam(const char *name)
{
    ENSURE_VALID(getpwnam, 0);

    int fd = open("/etc/passwd", O_RDONLY);
    undo_mask_start(fd);

    struct passwd *pw = lower_getpwnam(name);

    char buf[128];
    snprintf(&buf[0], sizeof(buf), "name:%s", name);
    undo_depend(fd, buf, "pwmgr", 0, 1);

    undo_mask_end(fd);
    close(fd);
    return pw;
}

struct passwd *
getpwuid(uid_t uid)
{
    ENSURE_VALID(getpwuid, 0);

    int fd = open("/etc/passwd", O_RDONLY);
    undo_mask_start(fd);

    struct passwd *pw = lower_getpwuid(uid);
    if (pw) {
	char buf[128];
	snprintf(&buf[0], sizeof(buf), "uid:%u", uid);
	undo_depend(fd, buf, "pwmgr", 0, 1);
    }

    undo_mask_end(fd);
    close(fd);
    return pw;
}

int
getpwnam_r(const char *name, struct passwd *pwd,
	   char *buf, size_t buflen, struct passwd **result)
{
    ENSURE_VALID(getpwnam_r, -1);

    int fd = open("/etc/passwd", O_RDONLY);
    undo_mask_start(fd);

    int r = lower_getpwnam_r(name, pwd, buf, buflen, result);
    if (r >= 0) {
	char buf[128];
	snprintf(&buf[0], sizeof(buf), "name:%s", name);
	undo_depend(fd, buf, "pwmgr", 0, 1);
    }

    undo_mask_end(fd);
    close(fd);
    return r;
}

int
getpwuid_r(uid_t uid, struct passwd *pwd,
	   char *buf, size_t buflen, struct passwd **result)
{
    ENSURE_VALID(getpwuid_r, -1);

    int fd = open("/etc/passwd", O_RDONLY);
    undo_mask_start(fd);

    int r = lower_getpwuid_r(uid, pwd, buf, buflen, result);
    if (r >= 0) {
	char buf[128];
	snprintf(&buf[0], sizeof(buf), "uid:%u", uid);
	undo_depend(fd, buf, "pwmgr", 0, 1);
    }

    undo_mask_end(fd);
    close(fd);
    return r;
}

// utmp

static void
utmp_depend(int fd, const struct utmp *ut, int proc_to_obj, int obj_to_proc)
{
    if (!ut)
	return;
    char buf[4+UT_LINESIZE+12];
    snprintf(buf, sizeof(buf), "%hu:%.4s:%.32s", ut->ut_type, ut->ut_id, ut->ut_line);
    undo_depend(fd, buf, "utmp_mgr", proc_to_obj, obj_to_proc);
}

struct utmp *
pututline(const struct utmp *ut)
{
    ENSURE_VALID(pututline, 0);
    int fd = open(UTMP_FILE, O_RDONLY);
    undo_mask_start(fd);

    const struct utmp *t = getutid(ut);
    if (!t) // new entry
	t = ut;
    utmp_depend(fd, t, 1, 0);
    struct utmp *r = lower_pututline(ut);

    undo_mask_end(fd);
    close(fd);
    return r;
}

struct utmp *
getutid(const struct utmp *ut)
{
    ENSURE_VALID(getutid, 0);

    int fd = open(UTMP_FILE, O_RDONLY);
    undo_mask_start(fd);

    struct utmp *r = lower_getutid(ut);
    utmp_depend(fd, r, 0, 1);

    undo_mask_end(fd);
    close(fd);
    return r;
}

int
getutid_r(const struct utmp *ut, struct utmp *ubuf, struct utmp **ubufp)
{
    ENSURE_VALID(getutid_r, -1);

    int fd = open(UTMP_FILE, O_RDONLY);
    undo_mask_start(fd);

    int r = lower_getutid_r(ut, ubuf, ubufp);
    utmp_depend(fd, *ubufp, 0, 1);

    undo_mask_end(fd);
    close(fd);
    return r;
}

UNDO_WRAP(getutline, struct utmp *, (const struct utmp * ut))
{
    int fd = open(UTMP_FILE, O_RDONLY);
    undo_mask_start(fd);

    struct utmp *r = UNDO_ORIG(getutline)(ut);
    utmp_depend(fd, r, 0, 1);

    undo_mask_end(fd);
    close(fd);
    return r;
}

UNDO_WRAP(getutline_r, int, (const struct utmp * ut, struct utmp *ubuf, struct utmp **ubufp))
{
    int fd = open(UTMP_FILE, O_RDONLY);
    undo_mask_start(fd);

    int r = UNDO_ORIG(getutline_r)(ut, ubuf, ubufp);
    utmp_depend(fd, *ubufp, 0, 1);

    undo_mask_end(fd);
    close(fd);
    return r;
}

UNDO_WRAP(getutent, struct utmp *, ())
{
    int fd = open(UTMP_FILE, O_RDONLY);
    undo_mask_start(fd);

    struct utmp *r = UNDO_ORIG(getutent)();
    utmp_depend(fd, r, 0, 1);

    undo_mask_end(fd);
    close(fd);
    return r;
}

UNDO_WRAP(getutent_r, int, (struct utmp *ubuf, struct utmp **ubufp))
{
    int fd = open(UTMP_FILE, O_RDONLY);
    undo_mask_start(fd);

    int r = UNDO_ORIG(getutent_r)(ubuf, ubufp);
    utmp_depend(fd, *ubufp, 0, 1);

    undo_mask_end(fd);
    close(fd);
    return r;
}

#if 0
UNDO_WRAP(updwtmp, void, (const char *wtmp_file, const struct utmp *ut))
{
    int fd = open(wtmp_file, O_RDONLY);
    undo_mask_start(fd);

    char buf[64];
    snprintf(buf, sizeof(buf), "%u:%llu", ut->ut_pid,
        ((unsigned long long)ut->ut_tv.tv_sec << 32) | ut->ut_tv.tv_usec);
    undo_depend(fd, buf, "wtmp_mgr", 1, 0);
    UNDO_ORIG(updwtmp)(wtmp_file, ut);

    undo_mask_end(fd);
    close(fd);
}
#endif

// don't intercept logwtmp

// NOTE: login is a higher-level call that changes both utmp and wtmp files
// intercept lower-level calls (e.g., pututline and updwtmp) instead
#if 0
void
login(const struct utmp *ut)
{
    ENSURE_VALID(login, );

    char buf[16];
    sprintf(&buf[0], "%.4s", ut->ut_id);

    int fd = open("/var/run/utmp", O_RDONLY);
    undo_mask_start(fd);

    undo_depend(fd, buf, "utmp_mgr", 1, 0);
    lower_login(ut);

    undo_mask_end(fd);
    close(fd);
}

int
logout(const char *ut_line)
{
    ENSURE_VALID(logout, -1);

    char buf[16];
    int len = strlen(ut_line);
    if (len < 4) len = 4;
    sprintf(&buf[0], "%.4s", ut_line + len - 4);

    int fd = open("/var/run/utmp", O_RDONLY);
    undo_mask_start(fd);

    undo_depend(fd, buf, "utmp_mgr", 1, 0);
    int r = lower_logout(ut_line);

    undo_mask_end(fd);
    close(fd);
    return r;
}
#endif
