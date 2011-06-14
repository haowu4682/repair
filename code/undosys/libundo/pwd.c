#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <string.h>
#include "undocall.h"

// only make actors here
// don't trust sub-objects

#define undo_pwd_start(l, s) \
    undo_func_start("pwdmgr", __func__, (l), (s))

static void
undo_pwd_end(struct passwd *pw)
{
    char buf[1024] = {0};
    if (pw)
        snprintf(buf, sizeof(buf), "%s:%s:%d:%d:%s:%s:%s", pw->pw_name, pw->pw_passwd,
            pw->pw_uid, pw->pw_gid, pw->pw_gecos, pw->pw_dir, pw->pw_shell);
    undo_func_end(strlen(buf), buf);
}

UNDO_WRAP(getpwnam, struct passwd *, (const char *name))
{
    undo_pwd_start(strlen(name), name);
    struct passwd *pw = UNDO_ORIG(getpwnam)(name);
    undo_pwd_end(pw);
    return pw;
}

UNDO_WRAP(getpwnam_r, int, (const char *name, struct passwd *pwd,
          char *buf, size_t buflen, struct passwd **result))
{
    undo_pwd_start(strlen(name), name);
    int r = UNDO_ORIG(getpwnam_r)(name, pwd, buf, buflen, result);
    undo_pwd_end(*result);
    return r;
}

UNDO_WRAP(getpwuid, struct passwd *, (uid_t uid))
{
    undo_pwd_start(sizeof(uid), &uid);
    struct passwd *pw = UNDO_ORIG(getpwuid)(uid);
    undo_pwd_end(pw);
    return pw;
}

UNDO_WRAP(getpwuid_r, int, (uid_t uid, struct passwd *pwd,
          char *buf, size_t buflen, struct passwd **result))
{
    undo_pwd_start(sizeof(uid), &uid);
    int r = UNDO_ORIG(getpwuid_r)(uid, pwd, buf, buflen, result);
    undo_pwd_end(*result);
    return r;
}

// shadow

#define undo_spwd_start undo_pwd_start

static void undo_spwd_end(struct spwd *sp)
{
    char buf[1024] = {0};
    if (sp) {
        char buf_inact[32] = {0}, buf_expire[32] = {0}, buf_flag[32] = {0};
        if (sp->sp_inact >= 0)
            sprintf(buf_inact, "%ld", sp->sp_inact);
        if (sp->sp_expire >= 0)
            sprintf(buf_expire, "%ld", sp->sp_expire);
        if (sp->sp_flag != -1)
            sprintf(buf_flag, "%lu", sp->sp_flag);
        snprintf(buf, sizeof(buf), "%s:%s:%ld:%ld:%ld:%ld:%s:%s:%s", sp->sp_namp,
            sp->sp_pwdp, sp->sp_lstchg, sp->sp_min, sp->sp_max, sp->sp_warn,
            buf_inact, buf_expire, buf_flag);
    }
    undo_func_end(strlen(buf), buf);
}

UNDO_WRAP(getspnam, struct spwd *, (const char *name))
{
    undo_spwd_start(strlen(name), name);
    struct spwd *sp = UNDO_ORIG(getspnam)(name);
    undo_spwd_end(sp);
    return sp;
}

UNDO_WRAP(getspnam_r, int, (const char *name, struct spwd *spbuf,
          char *buf, size_t buflen, struct spwd **spbufp))
{
    undo_spwd_start(strlen(name), name);
    int r = UNDO_ORIG(getspnam_r)(name, spbuf, buf, buflen, spbufp);
    undo_spwd_end(*spbufp);
    return r;
}

// group

#define undo_grp_start undo_pwd_start

static void undo_grp_end(struct group *gr)
{
    char buf[1024] = {0};
    if (gr) {
        snprintf(buf, sizeof(buf), "%s:%s:%d:", gr->gr_name, gr->gr_passwd, gr->gr_gid);
        for (char **p = gr->gr_mem; *p; ++p) {
            if (p != gr->gr_mem)
                strcat(buf, ",");
            strcat(buf, *p);
        }
    }
    undo_func_end(strlen(buf), buf);
}

UNDO_WRAP(getgrnam, struct group *, (const char *name))
{
    undo_grp_start(strlen(name), name);
    struct group *gr = UNDO_ORIG(getgrnam)(name);
    undo_grp_end(gr);
    return gr;
}

UNDO_WRAP(getgrnam_r, int, (const char *name, struct group *grp,
          char *buf, size_t buflen, struct group **result))
{
    undo_grp_start(strlen(name), name);
    int r = UNDO_ORIG(getgrnam_r)(name, grp, buf, buflen, result);
    undo_grp_end(*result);
    return r;
}

UNDO_WRAP(getgrgid, struct group *, (gid_t gid))
{
    undo_grp_start(sizeof(gid), &gid);
    struct group *gr = UNDO_ORIG(getgrgid)(gid);
    undo_grp_end(gr);
    return gr;
}

UNDO_WRAP(getgrgid_r, int, (gid_t gid, struct group *grp,
          char *buf, size_t buflen, struct group **result))
{
    undo_grp_start(sizeof(gid), &gid);
    int r = UNDO_ORIG(getgrgid_r)(gid, grp, buf, buflen, result);
    undo_grp_end(*result);
    return r;
}

// ugly hack: should have called getgrnam for TTY_GROUP
UNDO_WRAP(grantpt, int, (int fd))
{
    undo_func_start("pwdmgr", "grantpt", 0, 0);
    int r = UNDO_ORIG(grantpt)(fd);
    undo_func_end(sizeof(r), &r);
    return r;
}

// groups

UNDO_WRAP(initgroups, int, (const char *user, gid_t gid))
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s:%d", user, gid);
    undo_func_start("pwdmgr", "initgroups", strlen(buf), buf);
    int r = UNDO_ORIG(initgroups)(user, gid);
    undo_func_end(sizeof(r), &r);
    return r;
}

#if 0
UNDO_WRAP(getgrouplist, int, (const char *user, gid_t group,
          gid_t *groups, int *ngroups))
{
    undo_func_start("pwdmgr", "getgrouplist", 0, 0);
    int r = UNDO_ORIG(getgrouplist)(user, group, groups, ngroups);
    undo_func_end(0, 0);
    return r;
}

UNDO_WRAP(getgroups, int, (int size, gid_t *list))
{
    undo_func_start("pwdmgr", "getgroups", sizeof(int), &size);   
    int r = UNDO_ORIG(getgroups)(size, list);
    undo_func_end(0, 0);
    return r;
}

UNDO_WRAP(setgroups, int, (size_t size, const gid_t *list))
{
    undo_func_start("pwdmgr", "setgroups", size * sizeof(gid_t), list);
    int r = UNDO_ORIG(setgroups)(size, list);
    undo_func_end(0, 0);
    return r;
}
#endif
