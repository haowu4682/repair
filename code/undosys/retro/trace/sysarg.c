#include <linux/binfmts.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/uio.h>
#include <linux/uaccess.h>
#include <linux/socket.h>
#include <net/inet_sock.h>

#include "syscall.h"
#include "util.h"
#include "sha1.h"

#define FASTBUF         PAGE_SIZE

#ifndef MAX_ARG_STRINGS
#define MAX_ARG_STRINGS 0x7fffffff
#endif
#ifndef MAX_ARG_STRLEN
#define MAX_ARG_STRLEN  (PAGE_SIZE * 32)
#endif

#ifndef UIO_FASTIOV
#define UIO_FASTIOV     8
#endif
#ifndef UIO_MAXIOV
#define UIO_MAXIOV      1024
#endif

/* similar to __relay_write, but use copy_from_user instead of memcpy */
static inline void syscall_log_user(const void *data, size_t len)
{
  struct rchan_buf *buf = syscall_chan->buf[get_cpu()];
  if (unlikely(buf->offset + len > buf->chan->subbuf_size))
    len = relay_switch_subbuf(buf, len);
  if (copy_from_user(buf->data + buf->offset, data, len))
    ; /* suppress unused warning */
  buf->offset += len;
  put_cpu();
}

static inline void * syscall_malloc(size_t size)
{
  return kmalloc(size, GFP_KERNEL);
}

static inline void syscall_free(void *addr)
{
  return kfree(addr);
}

static inline size_t syscall_copy(void *buf, long addr, size_t len)
{
  return copy_from_user(buf, (void *)addr, len);
}

void sysarg_void(long v, const struct sysarg *arg) { }

void sysarg_sint(long v, const struct sysarg *arg)
{
  char buf[16];
  size_t len = svarint(v, buf);
  syscall_log(buf, len);
}

void sysarg_uint(long v, const struct sysarg *arg)
{
  char buf[16];
  size_t len = uvarint(v, buf);
  syscall_log(buf, len);
}

void sysarg_sint32(long v, const struct sysarg *arg)
{
  return sysarg_sint((int32_t)v, arg);
}

void sysarg_uint32(long v, const struct sysarg *arg)
{
  return sysarg_uint((uint32_t)v, arg);
}

void sysarg_pid_t(long v, const struct sysarg *arg)
{
  // XXX
  // when we are loading this system call records,
  // we don't know about the generation number of this
  // process without scanning all records of the process
  // holding the pid
  //

  // index_pid(v);
  return sysarg_sint32(v, arg);
}

void sysarg_intp(long v, const struct sysarg *arg)
{
  char buf[32];
  size_t len;
  if (v) {
    int r;
    if (syscall_copy(&r, v, sizeof(r)))
      goto err;
    len = uvarint(1, buf);
    len += svarint(r, buf + len);
    goto end;
  }
err:
  len = uvarint(0, NULL);
end:
  syscall_log(buf, len);
}

void sysarg_buf(long v, const struct sysarg *arg)
{
  size_t len = arg->aux;
  sysarg_uint(len, NULL);
  if (unlikely(len == 0))
    return;
  for (;;) {
    size_t size = (len < FASTBUF)? len: FASTBUF;
    syscall_log_user((const void *)v, size);
    if (len <= FASTBUF)
      break;
    len -= FASTBUF;
    v += FASTBUF;
  }
}

inline void sysarg_struct(long v, const struct sysarg *arg)
{
  if (v == 0) {
    sysarg_uint(0, NULL);
    return;
  }
  sysarg_buf(v, arg);
}

void sysarg_psize_t(long v, const struct sysarg *arg)
{
  int size = 0;
  get_user(size, (int *)v);
  sysarg_uint(size, arg);
}

void sysarg_msghdr(long v, const struct sysarg *dummy)
{
  struct sysarg arg = {0};
  struct msghdr msg_sys;
  struct msghdr __user *msg = (struct msghdr *)v;

  arg.aux = sizeof(struct msghdr);

  /* msghdr */
  if (copy_from_user(&msg_sys, msg, sizeof(struct msghdr))) {
    arg.aux = 0;
  }

  sysarg_buf(v, &arg);

  /* name */
  arg.aux = 12;
  sysarg_struct((long)msg_sys.msg_name, &arg);

  /* iov */
  arg.aux = msg_sys.msg_iovlen;
  sysarg_iovec((long)msg_sys.msg_iov, &arg);
}

void sysarg_buf_det(long v, const struct sysarg *arg)
{
  if (is_deterministic(current)) {
    dbg(det, "skip write:%d", (int)arg->aux);
    sysarg_uint(0, NULL);
    return;
  }

  sysarg_buf(v, arg);
  return;
}

void sysarg_sha1(long v, const struct sysarg *arg)
{
  long len = (long) arg->aux;
  unsigned char sha1[SHA1_SIZE];
  void * buf;

  if (unlikely(len <= 0 || is_deterministic(current))) {
    sysarg_uint(0, NULL);
    return;
  }

  // record buffer directly if < 128
  if (unlikely(len <= 128)) {
    sysarg_buf(v, arg);
    return;
  }

  buf = syscall_malloc(len);
  if (!buf) {
    sysarg_uint(0, NULL);
    return;
  }
  if (copy_from_user(buf, (const void *)v, len)) {
    goto free;
  }
  if (hmac_sha1(buf, len, sha1)) {
    memset(sha1, 0, SHA1_SIZE);
  }
  sysarg_uint(SHA1_SIZE, NULL);
  syscall_log(sha1, SHA1_SIZE);
 free:
  syscall_free(buf);
 }

void sysarg_string(long v, const struct sysarg *arg)
{
  char __user *str = (char *)v;
  long len;
  struct sysarg a = {0};
  if ( !str || !(len = strnlen_user(str, MAX_ARG_STRLEN))) {
    sysarg_uint(0, NULL);
    return;
  }
  a.aux = len - 1;
  sysarg_buf((long)str, &a);
}

void sysarg_strings(long v, const struct sysarg *arg)
{
  int n, i;
  char __user * __user * argv = (char **)v;

  n = count_argv(argv, MAX_ARG_STRINGS);
  if (unlikely(n < 0)) {
    sysarg_uint(0, NULL);
    return;
  }
  sysarg_uint(n, NULL);
  for (i = 0; i < n; ++i) {
    char __user * str;
    if (get_user(str, argv + i)) {
        str = NULL;
    }
    sysarg_string((long)str, arg);
  }
}

void sysarg_fd2(long v, const struct sysarg *arg)
{
  int fd[2];
  if (syscall_copy(fd, v, sizeof(fd))) {
    fd[0] = -1;
    fd[1] = -1;
  }
  sysarg_fd(fd[0], arg);
  sysarg_fd(fd[1], arg);
}

void sysarg_path(long v, const struct sysarg *arg)
{
  struct sysarg a = *arg;
  a.aux = AT_FDCWD;
  sysarg_path_at(v, &a);
}

void sysarg_rpath(long v, const struct sysarg *arg)
{
  struct sysarg a = *arg;
  a.aux = AT_FDCWD;
  sysarg_rpath_at(v, &a);
}

// XXX.
//
// need separate mechanisms for writev/readv syscalls
// - this should consider return value & argument at the same time
// - infrequently, the iov args are full of garbage from userspace
// - iov[i] should be filled before the iov[i+1]
// - so, we only have to read iov[] upto the valid length kernel returned
//
void sysarg_iovec(long v, const struct sysarg *arg)
{
  struct iovec iovstack[UIO_FASTIOV];
  struct iovec *iov;
  size_t count = arg->aux, i, size;
  long ret = arg->ret;

  // XXX
  // think it as not equivalent if bigger than 1024
  //
  if (unlikely(ret <= 0 || ret > 1024 || is_deterministic(current))) {
    sysarg_uint(0, NULL);
    return;
  }
  if (count > UIO_MAXIOV) {
    sysarg_uint(0, NULL);
    return;
  }
  if (count > UIO_FASTIOV) {
    iov = syscall_malloc(sizeof(*iov) * count);
    if (!iov) {
      sysarg_uint(0, NULL);
      return;
    }
  } else
    iov = iovstack;
  if (syscall_copy(iov, v, sizeof(*iov) * count))
    count = 0;

  // find effective count
  for ( i = 0, size = 0 ; i < count && size <= ret ; ++i ) {
    size += iov[i].iov_len;
  }
  count = i;

  // in reading: size == ret, but in writing: ret == LIMIT
  sysarg_uint(i, NULL);
  for (i = 0; i < count; ++i) {
    struct sysarg a = {0};
    a.aux = iov[i].iov_len;
    sysarg_sha1((long)iov[i].iov_base, &a);
  }

  if (iov != iovstack)
    syscall_free(iov);
}

static inline int __sockfd(struct file * file, char *buf)
{
  int family = AF_UNSPEC;
  int len = 0;
  struct socket *sock;
  if (!(file->f_op == NM_socket_file_ops)) {
    return 0;
  }

  if (!(sock = file->private_data))
    return 0;

  if (sock->ops && sock->ops->family) {
    family = sock->ops->family;
  }

  len += uvarint(family, buf);

  if (sock->ops && sock->ops->family == AF_INET) {
    struct inet_sock *inet = inet_sk(sock->sk);
    if (inet) {
      len += uvarint(inet->inet_sport, buf+len);
      len += uvarint(inet->inet_dport, buf+len);
      len += uvarint(inet->inet_daddr, buf+len);

      index_sock(inet->inet_daddr, inet->inet_dport);
    }
  }

  return len;
}

void sysarg_fd(long v, const struct sysarg *arg)
{
  char buf[256];
  size_t len;
  int fd = (int)v;
  struct file *file;
  int fput_needed;
  struct inode *inode;
  umode_t mode;

  len = svarint(fd, buf);
  if (fd < 0)
    goto end;
  file = fget_light(fd, &fput_needed);
  if (!file) {
    len += uvarint(0, buf + len);
    goto end;
  }

  inode = file->f_path.dentry->d_inode;
  index_inode(inode);
  len += varinode(inode, buf + len);
  /* /dev/ptmx */
  if (inode->i_rdev == MKDEV(TTYAUX_MAJOR, 2)) {
    struct tty_struct *tty = file->private_data;
    len += svarint(tty->link->index, buf + len);
  }
  /* offset */
  if (arg && arg->aux){
    len += svarint(file->f_flags & O_APPEND ? -1 : file->f_pos,
             buf + len);
  }

  mode = inode->i_mode;

  /* socket */
  if (S_ISSOCK(mode)) {
    len += __sockfd(file, buf + len);
  }

  fput_light(file, fput_needed);
end:
  syscall_log(buf, len);
}

static inline void ksysarg_string(const char *s)
{
  size_t len = strlen(s);
        sysarg_uint(len, NULL);
        syscall_log(s, len);
}

void sysarg_name(long v, const struct sysarg *arg)
{
  char *s = getname((const char *)v);
  if (IS_ERR(s)) {
    sysarg_uint(0, NULL);
    return;
  }
  ksysarg_string(s);
  putname(s);
}

static void ksysarg_dentry(struct inode *inode, const char *name, size_t namesize)
{
  char buf[128];
  size_t len;

  index_inode(inode);
  len = varinode(inode, buf);
  len += uvarint(namesize, buf + len);
  syscall_log(buf, len);
  syscall_log(name, namesize);
}

#include "namei.c"

static void ksysarg_namei(long v, const struct sysarg *arg, int flags)
{
  int dfd = (int)arg->aux;
  char *s;
  struct nameidata nd;
  int err;
  struct fs_struct *fs = current->fs;
  struct dentry *root;

  s = getname((const char *)v);
  if (IS_ERR(s))
    goto end;
  
  ksysarg_string(s);
  
  // to support chroot, record root
  fs_lock(fs);
  root = fs->root.dentry;
  fs_unlock(fs);

  ksysarg_dentry(root->d_inode, root->d_name.name, root->d_name.len);
  
  reset_pathid();
  err = do_path_lookup(dfd, s, flags, &nd);
  index_pathid(last_pathid());
  syscall_log(last_pathid(), PATHID_LEN);
  putname(s);
  if (!err) {
    char buf[128];
    size_t len;
    struct inode *inode = nd.path.dentry->d_inode;
  
    index_inode(inode);
    len = varinode(inode, buf);
    syscall_log(buf, len);
    path_put(&nd.path);

    return;
  }
end:
  sysarg_uint(0, NULL);
}

void sysarg_path_at(long v, const struct sysarg *arg)
{
  ksysarg_namei(v, arg, LOOKUP_FOLLOW);
}

void sysarg_rpath_at(long v, const struct sysarg *arg)
{
  ksysarg_namei(v, arg, 0);
}

void sysarg_execve(long v, const struct sysarg *arg)
{
  struct fs_struct *fs = current->fs;
  struct files_struct *files = current->files;
  struct dentry *pwd, *root;
  struct fdtable *fdt;
  size_t i;

  sysarg_sint32(v, NULL);
  if ((int)v)
    return;

  fs_lock(fs);
  root = fs->root.dentry;
  pwd = fs->pwd.dentry;
  fs_unlock(fs);
  
  ksysarg_dentry(root->d_inode, root->d_name.name, root->d_name.len);
  ksysarg_dentry(pwd->d_inode, pwd->d_name.name, pwd->d_name.len);

  spin_lock(&files->file_lock);
  fdt = files_fdtable(files);
  for (i = 0; i < fdt->max_fds; ++i) {
    struct file * file = rcu_dereference(fdt->fd[i]);
    if (file) {
      char buf[128];
      size_t len;
      struct inode *inode = file->f_path.dentry->d_inode;

      index_inode(inode);
      len = svarint(i, buf);
      len += varinode(inode, buf + len);
      /* socket */
      if (S_ISSOCK(inode->i_mode)) {
        len += __sockfd(file, buf + len);
      }
      len += svarint(file->f_pos, buf + len);
      len += svarint(file->f_flags, buf + len);
      syscall_log(buf, len);
    }
  }
  sysarg_sint(-1, NULL);
  spin_unlock(&files->file_lock);
}
