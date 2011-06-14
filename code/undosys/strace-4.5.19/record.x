struct stat_info {
    hyper dev;
    hyper ino;
    hyper gen;

    int mode;
    int rdev;

    /* for /dev/ptmx demultiplexing */
    int ptsid;

    /* not really associated with an inode, but.. */
    int fd_offset;
    int fd_flags;
};

struct namei_info {
    hyper dev;
    hyper ino;
    hyper gen;
    string name<>;
};

struct namei_infos {
    namei_info ni<>;
};

struct argv_str {
    string s<>;
};

struct syscall_arg {
    opaque data<>;
    argv_str argv<>;
    namei_infos *namei;
    stat_info *fd_stat;
};

struct exec_info {
    stat_info cwd;
    stat_info root;
    stat_info fds<>;
};

struct dirent_infos {
    hyper dev;
    hyper ino;
    hyper gen;
    namei_info ni<>;
};

struct snap_info {
    stat_info *file_stat;
    hyper id;
    dirent_infos dirents<2>;
};

struct syscall_record {
    int pid;
    int scno;
    string scname<>;

    bool enter;
    syscall_arg args<>;
    hyper ret;
    hyper err;

    stat_info *ret_fd_stat;
    exec_info *execi;

    snap_info snap;	/* Just before this system call happened */
};
