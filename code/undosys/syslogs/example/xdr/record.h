/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _RECORD_H_RPCGEN
#define _RECORD_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif


struct stat_info {
	quad_t dev;
	quad_t ino;
	quad_t gen;
	int mode;
	int rdev;
	int ptsid;
	int fd_offset;
};
typedef struct stat_info stat_info;

struct namei_info {
	quad_t dev;
	quad_t ino;
	quad_t gen;
	char *name;
};
typedef struct namei_info namei_info;

struct namei_infos {
	struct {
		u_int ni_len;
		namei_info *ni_val;
	} ni;
};
typedef struct namei_infos namei_infos;

struct argv_str {
	char *s;
};
typedef struct argv_str argv_str;

struct syscall_arg {
	struct {
		u_int data_len;
		char *data_val;
	} data;
	struct {
		u_int argv_len;
		argv_str *argv_val;
	} argv;
	namei_infos *namei;
	stat_info *fd_stat;
};
typedef struct syscall_arg syscall_arg;

struct syscall_record {
	int pid;
	int scno;
	char *scname;
	bool_t enter;
	struct {
		u_int args_len;
		syscall_arg *args_val;
	} args;
	quad_t ret;
	quad_t err;
	stat_info *ret_fd_stat;
	quad_t snap_id;
};
typedef struct syscall_record syscall_record;

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_stat_info (XDR *, stat_info*);
extern  bool_t xdr_namei_info (XDR *, namei_info*);
extern  bool_t xdr_namei_infos (XDR *, namei_infos*);
extern  bool_t xdr_argv_str (XDR *, argv_str*);
extern  bool_t xdr_syscall_arg (XDR *, syscall_arg*);
extern  bool_t xdr_syscall_record (XDR *, syscall_record*);

#else /* K&R C */
extern bool_t xdr_stat_info ();
extern bool_t xdr_namei_info ();
extern bool_t xdr_namei_infos ();
extern bool_t xdr_argv_str ();
extern bool_t xdr_syscall_arg ();
extern bool_t xdr_syscall_record ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_RECORD_H_RPCGEN */
