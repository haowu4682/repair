/*
 * Copyright (c) 1991, 1992 Paul Kranenburg <pk@cs.few.eur.nl>
 * Copyright (c) 1993 Branko Lankester <branko@hacktic.nl>
 * Copyright (c) 1993, 1994, 1995, 1996 Rick Sladkey <jrs@world.std.com>
 * Copyright (c) 1996-1999 Wichert Akkerman <wichert@cistron.nl>
 * Copyright (c) 1999 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *                     Linux for s390 port by D.J. Barrow
 *                    <barrow_dj@mail.yahoo.com,djbarrow@de.ibm.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id$
 */

#include "defs.h"
#include "sysarg.h"
#include "record.h"
#include "undocall.h"

#include <assert.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <linux/fs.h>

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#ifndef PTRACE_PEEKUSR
# define PTRACE_PEEKUSR PTRACE_PEEKUSER
#endif
#elif defined(HAVE_LINUX_PTRACE_H)
#undef PTRACE_SYSCALL
# ifdef HAVE_STRUCT_IA64_FPREG
#  define ia64_fpreg XXX_ia64_fpreg
# endif
# ifdef HAVE_STRUCT_PT_ALL_USER_REGS
#  define pt_all_user_regs XXX_pt_all_user_regs
# endif
#include <linux/ptrace.h>
# undef ia64_fpreg
# undef pt_all_user_regs
#endif

#if defined (LINUX) && defined (SPARC64)
# undef PTRACE_GETREGS
# define PTRACE_GETREGS PTRACE_GETREGS64
# undef PTRACE_SETREGS
# define PTRACE_SETREGS PTRACE_SETREGS64
#endif /* LINUX && SPARC64 */

#if defined(LINUX) && defined(IA64)
# include <asm/ptrace_offsets.h>
# include <asm/rse.h>
#endif

#define NR_SYSCALL_BASE 0
#ifdef LINUX
#ifndef ERESTARTSYS
#define ERESTARTSYS	512
#endif
#ifndef ERESTARTNOINTR
#define ERESTARTNOINTR	513
#endif
#ifndef ERESTARTNOHAND
#define ERESTARTNOHAND	514	/* restart if no handler.. */
#endif
#ifndef ENOIOCTLCMD
#define ENOIOCTLCMD	515	/* No ioctl command */
#endif
#ifndef ERESTART_RESTARTBLOCK
#define ERESTART_RESTARTBLOCK 516	/* restart by calling sys_restart_syscall */
#endif
#ifndef NSIG
#define NSIG 32
#endif
#ifdef ARM
#undef NSIG
#define NSIG 32
#undef NR_SYSCALL_BASE
#define NR_SYSCALL_BASE __NR_SYSCALL_BASE
#endif
#endif /* LINUX */

#include "syscall.h"

/* Define these shorthand notations to simplify the syscallent files. */
#define TD TRACE_DESC
#define TF TRACE_FILE
#define TI TRACE_IPC
#define TN TRACE_NETWORK
#define TP TRACE_PROCESS
#define TS TRACE_SIGNAL

static const struct sysent sysent0[] = {
#include "syscallent.h"
};
static const int nsyscalls0 = sizeof sysent0 / sizeof sysent0[0];
int qual_flags0[MAX_QUALS];

#if SUPPORTED_PERSONALITIES >= 2
static const struct sysent sysent1[] = {
#include "syscallent1.h"
};
static const int nsyscalls1 = sizeof sysent1 / sizeof sysent1[0];
int qual_flags1[MAX_QUALS];
#endif /* SUPPORTED_PERSONALITIES >= 2 */

#if SUPPORTED_PERSONALITIES >= 3
static const struct sysent sysent2[] = {
#include "syscallent2.h"
};
static const int nsyscalls2 = sizeof sysent2 / sizeof sysent2[0];
int qual_flags2[MAX_QUALS];
#endif /* SUPPORTED_PERSONALITIES >= 3 */

const struct sysent *sysent;
int *qual_flags;
int nsyscalls;

/* Now undef them since short defines cause wicked namespace pollution. */
#undef TD
#undef TF
#undef TI
#undef TN
#undef TP
#undef TS

static const char *const errnoent0[] = {
#include "errnoent.h"
};
static const int nerrnos0 = sizeof errnoent0 / sizeof errnoent0[0];

#if SUPPORTED_PERSONALITIES >= 2
static const char *const errnoent1[] = {
#include "errnoent1.h"
};
static const int nerrnos1 = sizeof errnoent1 / sizeof errnoent1[0];
#endif /* SUPPORTED_PERSONALITIES >= 2 */

#if SUPPORTED_PERSONALITIES >= 3
static const char *const errnoent2[] = {
#include "errnoent2.h"
};
static const int nerrnos2 = sizeof errnoent2 / sizeof errnoent2[0];
#endif /* SUPPORTED_PERSONALITIES >= 3 */

const char *const *errnoent;
int nerrnos;

int current_personality;

#ifndef PERSONALITY0_WORDSIZE
# define PERSONALITY0_WORDSIZE sizeof(long)
#endif
const int personality_wordsize[SUPPORTED_PERSONALITIES] = {
	PERSONALITY0_WORDSIZE,
#if SUPPORTED_PERSONALITIES > 1
	PERSONALITY1_WORDSIZE,
#endif
#if SUPPORTED_PERSONALITIES > 2
	PERSONALITY2_WORDSIZE,
#endif
};;

int
set_personality(int personality)
{
	switch (personality) {
	case 0:
		errnoent = errnoent0;
		nerrnos = nerrnos0;
		sysent = sysent0;
		nsyscalls = nsyscalls0;
		ioctlent = ioctlent0;
		nioctlents = nioctlents0;
		signalent = signalent0;
		nsignals = nsignals0;
		qual_flags = qual_flags0;
		break;

#if SUPPORTED_PERSONALITIES >= 2
	case 1:
		errnoent = errnoent1;
		nerrnos = nerrnos1;
		sysent = sysent1;
		nsyscalls = nsyscalls1;
		ioctlent = ioctlent1;
		nioctlents = nioctlents1;
		signalent = signalent1;
		nsignals = nsignals1;
		qual_flags = qual_flags1;
		break;
#endif /* SUPPORTED_PERSONALITIES >= 2 */

#if SUPPORTED_PERSONALITIES >= 3
	case 2:
		errnoent = errnoent2;
		nerrnos = nerrnos2;
		sysent = sysent2;
		nsyscalls = nsyscalls2;
		ioctlent = ioctlent2;
		nioctlents = nioctlents2;
		signalent = signalent2;
		nsignals = nsignals2;
		qual_flags = qual_flags2;
		break;
#endif /* SUPPORTED_PERSONALITIES >= 3 */

	default:
		return -1;
	}

	current_personality = personality;
	return 0;
}


static int qual_syscall(), qual_signal(), qual_fault(), qual_desc();

static const struct qual_options {
	int bitflag;
	char *option_name;
	int (*qualify)();
	char *argument_name;
} qual_options[] = {
	{ QUAL_TRACE,	"trace",	qual_syscall,	"system call"	},
	{ QUAL_TRACE,	"t",		qual_syscall,	"system call"	},
	{ QUAL_ABBREV,	"abbrev",	qual_syscall,	"system call"	},
	{ QUAL_ABBREV,	"a",		qual_syscall,	"system call"	},
	{ QUAL_VERBOSE,	"verbose",	qual_syscall,	"system call"	},
	{ QUAL_VERBOSE,	"v",		qual_syscall,	"system call"	},
	{ QUAL_RAW,	"raw",		qual_syscall,	"system call"	},
	{ QUAL_RAW,	"x",		qual_syscall,	"system call"	},
	{ QUAL_SIGNAL,	"signal",	qual_signal,	"signal"	},
	{ QUAL_SIGNAL,	"signals",	qual_signal,	"signal"	},
	{ QUAL_SIGNAL,	"s",		qual_signal,	"signal"	},
	{ QUAL_FAULT,	"fault",	qual_fault,	"fault"		},
	{ QUAL_FAULT,	"faults",	qual_fault,	"fault"		},
	{ QUAL_FAULT,	"m",		qual_fault,	"fault"		},
	{ QUAL_READ,	"read",		qual_desc,	"descriptor"	},
	{ QUAL_READ,	"reads",	qual_desc,	"descriptor"	},
	{ QUAL_READ,	"r",		qual_desc,	"descriptor"	},
	{ QUAL_WRITE,	"write",	qual_desc,	"descriptor"	},
	{ QUAL_WRITE,	"writes",	qual_desc,	"descriptor"	},
	{ QUAL_WRITE,	"w",		qual_desc,	"descriptor"	},
	{ 0,		NULL,		NULL,		NULL		},
};

static void
qualify_one(n, opt, not, pers)
	int n;
	const struct qual_options *opt;
	int not;
	int pers;
{
	if (pers == 0 || pers < 0) {
		if (not)
			qual_flags0[n] &= ~opt->bitflag;
		else
			qual_flags0[n] |= opt->bitflag;
	}

#if SUPPORTED_PERSONALITIES >= 2
	if (pers == 1 || pers < 0) {
		if (not)
			qual_flags1[n] &= ~opt->bitflag;
		else
			qual_flags1[n] |= opt->bitflag;
	}
#endif /* SUPPORTED_PERSONALITIES >= 2 */

#if SUPPORTED_PERSONALITIES >= 3
	if (pers == 2 || pers < 0) {
		if (not)
			qual_flags2[n] &= ~opt->bitflag;
		else
			qual_flags2[n] |= opt->bitflag;
	}
#endif /* SUPPORTED_PERSONALITIES >= 3 */
}

static int
qual_syscall(s, opt, not)
	char *s;
	const struct qual_options *opt;
	int not;
{
	int i;
	int rc = -1;

	if (isdigit((unsigned char)*s)) {
		int i = atoi(s);
		if (i < 0 || i >= MAX_QUALS)
			return -1;
		qualify_one(i, opt, not, -1);
		return 0;
	}
	for (i = 0; i < nsyscalls0; i++)
		if (strcmp(s, sysent0[i].sys_name) == 0) {
			qualify_one(i, opt, not, 0);
			rc = 0;
		}

#if SUPPORTED_PERSONALITIES >= 2
	for (i = 0; i < nsyscalls1; i++)
		if (strcmp(s, sysent1[i].sys_name) == 0) {
			qualify_one(i, opt, not, 1);
			rc = 0;
		}
#endif /* SUPPORTED_PERSONALITIES >= 2 */

#if SUPPORTED_PERSONALITIES >= 3
	for (i = 0; i < nsyscalls2; i++)
		if (strcmp(s, sysent2[i].sys_name) == 0) {
			qualify_one(i, opt, not, 2);
			rc = 0;
		}
#endif /* SUPPORTED_PERSONALITIES >= 3 */

	return rc;
}

static int
qual_signal(s, opt, not)
	char *s;
	const struct qual_options *opt;
	int not;
{
	int i;
	char buf[32];

	if (isdigit((unsigned char)*s)) {
		int signo = atoi(s);
		if (signo < 0 || signo >= MAX_QUALS)
			return -1;
		qualify_one(signo, opt, not, -1);
		return 0;
	}
	if (strlen(s) >= sizeof buf)
		return -1;
	strcpy(buf, s);
	s = buf;
	for (i = 0; s[i]; i++)
		s[i] = toupper((unsigned char)(s[i]));
	if (strncmp(s, "SIG", 3) == 0)
		s += 3;
	for (i = 0; i <= NSIG; i++)
		if (strcmp(s, signame(i) + 3) == 0) {
			qualify_one(i, opt, not, -1);
			return 0;
		}
	return -1;
}

static int
qual_fault(s, opt, not)
	char *s;
	const struct qual_options *opt;
	int not;
{
	return -1;
}

static int
qual_desc(s, opt, not)
	char *s;
	const struct qual_options *opt;
	int not;
{
	if (isdigit((unsigned char)*s)) {
		int desc = atoi(s);
		if (desc < 0 || desc >= MAX_QUALS)
			return -1;
		qualify_one(desc, opt, not, -1);
		return 0;
	}
	return -1;
}

static int
lookup_class(s)
	char *s;
{
	if (strcmp(s, "file") == 0)
		return TRACE_FILE;
	if (strcmp(s, "ipc") == 0)
		return TRACE_IPC;
	if (strcmp(s, "network") == 0)
		return TRACE_NETWORK;
	if (strcmp(s, "process") == 0)
		return TRACE_PROCESS;
	if (strcmp(s, "signal") == 0)
		return TRACE_SIGNAL;
	if (strcmp(s, "desc") == 0)
		return TRACE_DESC;
	return -1;
}

void
qualify(s)
char *s;
{
	const struct qual_options *opt;
	int not;
	char *p;
	int i, n;

	opt = &qual_options[0];
	for (i = 0; (p = qual_options[i].option_name); i++) {
		n = strlen(p);
		if (strncmp(s, p, n) == 0 && s[n] == '=') {
			opt = &qual_options[i];
			s += n + 1;
			break;
		}
	}
	not = 0;
	if (*s == '!') {
		not = 1;
		s++;
	}
	if (strcmp(s, "none") == 0) {
		not = 1 - not;
		s = "all";
	}
	if (strcmp(s, "all") == 0) {
		for (i = 0; i < MAX_QUALS; i++) {
			qualify_one(i, opt, not, -1);
		}
		return;
	}
	for (i = 0; i < MAX_QUALS; i++) {
		qualify_one(i, opt, !not, -1);
	}
	for (p = strtok(s, ","); p; p = strtok(NULL, ",")) {
		if (opt->bitflag == QUAL_TRACE && (n = lookup_class(p)) > 0) {
			for (i = 0; i < nsyscalls0; i++)
				if (sysent0[i].sys_flags & n)
					qualify_one(i, opt, not, 0);

#if SUPPORTED_PERSONALITIES >= 2
			for (i = 0; i < nsyscalls1; i++)
				if (sysent1[i].sys_flags & n)
					qualify_one(i, opt, not, 1);
#endif /* SUPPORTED_PERSONALITIES >= 2 */

#if SUPPORTED_PERSONALITIES >= 3
			for (i = 0; i < nsyscalls2; i++)
				if (sysent2[i].sys_flags & n)
					qualify_one(i, opt, not, 2);
#endif /* SUPPORTED_PERSONALITIES >= 3 */

			continue;
		}
		if (opt->qualify(p, opt, not)) {
			fprintf(stderr, "strace: invalid %s `%s'\n",
				opt->argument_name, p);
			exit(1);
		}
	}
	return;
}

static void
dumpio(tcp)
struct tcb *tcp;
{
	if (syserror(tcp))
		return;
	if (tcp->u_arg[0] < 0 || tcp->u_arg[0] >= MAX_QUALS)
		return;
	switch (known_scno(tcp)) {
	case SYS_read:
#ifdef SYS_pread64
	case SYS_pread64:
#endif
#if defined SYS_pread && SYS_pread64 != SYS_pread
	case SYS_pread:
#endif
#ifdef SYS_recv
	case SYS_recv:
#elif defined SYS_sub_recv
	case SYS_sub_recv:
#endif
#ifdef SYS_recvfrom
	case SYS_recvfrom:
#elif defined SYS_sub_recvfrom
	case SYS_sub_recvfrom:
#endif
		if (qual_flags[tcp->u_arg[0]] & QUAL_READ)
			dumpstr(tcp, tcp->u_arg[1], tcp->u_rval);
		break;
	case SYS_write:
#ifdef SYS_pwrite64
	case SYS_pwrite64:
#endif
#if defined SYS_pwrite && SYS_pwrite64 != SYS_pwrite
	case SYS_pwrite:
#endif
#ifdef SYS_send
	case SYS_send:
#elif defined SYS_sub_send
	case SYS_sub_send:
#endif
#ifdef SYS_sendto
	case SYS_sendto:
#elif defined SYS_sub_sendto
	case SYS_sub_sendto:
#endif
		if (qual_flags[tcp->u_arg[0]] & QUAL_WRITE)
			dumpstr(tcp, tcp->u_arg[1], tcp->u_arg[2]);
		break;
#ifdef SYS_readv
	case SYS_readv:
		if (qual_flags[tcp->u_arg[0]] & QUAL_READ)
			dumpiov(tcp, tcp->u_arg[2], tcp->u_arg[1]);
		break;
#endif
#ifdef SYS_writev
	case SYS_writev:
		if (qual_flags[tcp->u_arg[0]] & QUAL_WRITE)
			dumpiov(tcp, tcp->u_arg[2], tcp->u_arg[1]);
		break;
#endif
	}
}

#ifndef FREEBSD
enum subcall_style { shift_style, deref_style, mask_style, door_style };
#else /* FREEBSD */
enum subcall_style { shift_style, deref_style, mask_style, door_style, table_style };

struct subcall {
  int call;
  int nsubcalls;
  int subcalls[5];
};

static const struct subcall subcalls_table[] = {
  { SYS_shmsys, 5, { SYS_shmat, SYS_shmctl, SYS_shmdt, SYS_shmget, SYS_shmctl } },
#ifdef SYS_semconfig
  { SYS_semsys, 4, { SYS___semctl, SYS_semget, SYS_semop, SYS_semconfig } },
#else
  { SYS_semsys, 3, { SYS___semctl, SYS_semget, SYS_semop } },
#endif
  { SYS_msgsys, 4, { SYS_msgctl, SYS_msgget, SYS_msgsnd, SYS_msgrcv } },
};
#endif /* FREEBSD */

#if !(defined(LINUX) && ( defined(ALPHA) || defined(MIPS) || defined(__ARM_EABI__) ))

static void
decode_subcall(tcp, subcall, nsubcalls, style)
struct tcb *tcp;
int subcall;
int nsubcalls;
enum subcall_style style;
{
	unsigned long addr, mask;
	int i;
	int size = personality_wordsize[current_personality];

	switch (style) {
	case shift_style:
		if (tcp->u_arg[0] < 0 || tcp->u_arg[0] >= nsubcalls)
			return;
		tcp->scno = subcall + tcp->u_arg[0];
		if (sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs--;
		for (i = 0; i < tcp->u_nargs; i++)
			tcp->u_arg[i] = tcp->u_arg[i + 1];
		break;
	case deref_style:
		if (tcp->u_arg[0] < 0 || tcp->u_arg[0] >= nsubcalls)
			return;
		tcp->scno = subcall + tcp->u_arg[0];
		addr = tcp->u_arg[1];
		for (i = 0; i < sysent[tcp->scno].nargs; i++) {
			if (size == sizeof(int)) {
				unsigned int arg;
				if (umove(tcp, addr, &arg) < 0)
					arg = 0;
				tcp->u_arg[i] = arg;
			}
			else if (size == sizeof(long)) {
				unsigned long arg;
				if (umove(tcp, addr, &arg) < 0)
					arg = 0;
				tcp->u_arg[i] = arg;
			}
			else
				abort();
			addr += size;
		}
		tcp->u_nargs = sysent[tcp->scno].nargs;
		break;
	case mask_style:
		mask = (tcp->u_arg[0] >> 8) & 0xff;
		for (i = 0; mask; i++)
			mask >>= 1;
		if (i >= nsubcalls)
			return;
		tcp->u_arg[0] &= 0xff;
		tcp->scno = subcall + i;
		if (sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		break;
	case door_style:
		/*
		 * Oh, yuck.  The call code is the *sixth* argument.
		 * (don't you mean the *last* argument? - JH)
		 */
		if (tcp->u_arg[5] < 0 || tcp->u_arg[5] >= nsubcalls)
			return;
		tcp->scno = subcall + tcp->u_arg[5];
		if (sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs--;
		break;
#ifdef FREEBSD
	case table_style:
		for (i = 0; i < sizeof(subcalls_table) / sizeof(struct subcall); i++)
			if (subcalls_table[i].call == tcp->scno) break;
		if (i < sizeof(subcalls_table) / sizeof(struct subcall) &&
		    tcp->u_arg[0] >= 0 && tcp->u_arg[0] < subcalls_table[i].nsubcalls) {
			tcp->scno = subcalls_table[i].subcalls[tcp->u_arg[0]];
			for (i = 0; i < tcp->u_nargs; i++)
				tcp->u_arg[i] = tcp->u_arg[i + 1];
		}
		break;
#endif /* FREEBSD */
	}
}
#endif

struct tcb *tcp_last = NULL;

static int
internal_syscall(struct tcb *tcp)
{
	/*
	 * We must always trace a few critical system calls in order to
	 * correctly support following forks in the presence of tracing
	 * qualifiers.
	 */
	int	(*func)();

	if (tcp->scno < 0 || tcp->scno >= nsyscalls)
		return 0;

	func = sysent[tcp->scno].sys_func;

	if (sys_exit == func)
		return internal_exit(tcp);

	if (   sys_fork == func
#if defined(FREEBSD) || defined(LINUX) || defined(SUNOS4)
	    || sys_vfork == func
#endif
#if UNIXWARE > 2
	    || sys_rfork == func
#endif
	   )
		return internal_fork(tcp);

#if defined(LINUX) && (defined SYS_clone || defined SYS_clone2)
	if (sys_clone == func)
		return internal_clone(tcp);
#endif

	if (   sys_execve == func
#if defined(SPARC) || defined(SPARC64) || defined(SUNOS4)
	    || sys_execv == func
#endif
#if UNIXWARE > 2
	    || sys_rexecve == func
#endif
	   )
		return internal_exec(tcp);

	if (   sys_waitpid == func
	    || sys_wait4 == func
#if defined(SVR4) || defined(FREEBSD) || defined(SUNOS4)
	    || sys_wait == func
#endif
#ifdef ALPHA
	    || sys_osf_wait4 == func
#endif
	   )
		return internal_wait(tcp, 2);

#if defined(LINUX) || defined(SVR4)
	if (sys_waitid == func)
		return internal_wait(tcp, 3);
#endif

	return 0;
}


#ifdef LINUX
#if defined (I386)
	static long eax;
#elif defined (IA64)
	long r8, r10, psr;
	long ia32 = 0;
#elif defined (POWERPC)
	static long result,flags;
#elif defined (M68K)
	static int d0;
#elif defined(BFIN)
	static long r0;
#elif defined (ARM)
	static struct pt_regs regs;
#elif defined (ALPHA)
	static long r0;
	static long a3;
#elif defined(AVR32)
	static struct pt_regs regs;
#elif defined (SPARC) || defined (SPARC64)
	static struct pt_regs regs;
	static unsigned long trap;
#elif defined(LINUX_MIPSN32)
	static long long a3;
	static long long r2;
#elif defined(MIPS)
	static long a3;
	static long r2;
#elif defined(S390) || defined(S390X)
	static long gpr2;
	static long pc;
	static long syscall_mode;
#elif defined(HPPA)
	static long r28;
#elif defined(SH)
	static long r0;
#elif defined(SH64)
	static long r9;
#elif defined(X86_64)
	static long rax;
#elif defined(CRISV10) || defined(CRISV32)
	static long r10;
#endif
#endif /* LINUX */
#ifdef FREEBSD
	struct reg regs;
#endif /* FREEBSD */

int
get_scno(struct tcb *tcp)
{
	long scno = 0;

#ifdef LINUX
# if defined(S390) || defined(S390X)
	if (tcp->flags & TCB_WAITEXECVE) {
		/*
		 * When the execve system call completes successfully, the
		 * new process still has -ENOSYS (old style) or __NR_execve
		 * (new style) in gpr2.  We cannot recover the scno again
		 * by disassembly, because the image that executed the
		 * syscall is gone now.  Fortunately, we don't want it.  We
		 * leave the flag set so that syscall_fixup can fake the
		 * result.
		 */
		if (tcp->flags & TCB_INSYSCALL)
			return 1;
		/*
		 * This is the SIGTRAP after execve.  We cannot try to read
		 * the system call here either.
		 */
		tcp->flags &= ~TCB_WAITEXECVE;
		return 0;
	}

	if (upeek(tcp, PT_GPR2, &syscall_mode) < 0)
			return -1;

	if (syscall_mode != -ENOSYS) {
		/*
		 * Since kernel version 2.5.44 the scno gets passed in gpr2.
		 */
		scno = syscall_mode;
	} else {
		/*
		 * Old style of "passing" the scno via the SVC instruction.
		 */

		long opcode, offset_reg, tmp;
		void * svc_addr;
		int gpr_offset[16] = {PT_GPR0,  PT_GPR1,  PT_ORIGGPR2, PT_GPR3,
				      PT_GPR4,  PT_GPR5,  PT_GPR6,     PT_GPR7,
				      PT_GPR8,  PT_GPR9,  PT_GPR10,    PT_GPR11,
				      PT_GPR12, PT_GPR13, PT_GPR14,    PT_GPR15};

		if (upeek(tcp, PT_PSWADDR, &pc) < 0)
			return -1;
		errno = 0;
		opcode = ptrace(PTRACE_PEEKTEXT, tcp->pid, (char *)(pc-sizeof(long)), 0);
		if (errno) {
			perror("peektext(pc-oneword)");
			return -1;
		}

		/*
		 *  We have to check if the SVC got executed directly or via an
		 *  EXECUTE instruction. In case of EXECUTE it is necessary to do
		 *  instruction decoding to derive the system call number.
		 *  Unfortunately the opcode sizes of EXECUTE and SVC are differently,
		 *  so that this doesn't work if a SVC opcode is part of an EXECUTE
		 *  opcode. Since there is no way to find out the opcode size this
		 *  is the best we can do...
		 */

		if ((opcode & 0xff00) == 0x0a00) {
			/* SVC opcode */
			scno = opcode & 0xff;
		}
		else {
			/* SVC got executed by EXECUTE instruction */

			/*
			 *  Do instruction decoding of EXECUTE. If you really want to
			 *  understand this, read the Principles of Operations.
			 */
			svc_addr = (void *) (opcode & 0xfff);

			tmp = 0;
			offset_reg = (opcode & 0x000f0000) >> 16;
			if (offset_reg && (upeek(tcp, gpr_offset[offset_reg], &tmp) < 0))
				return -1;
			svc_addr += tmp;

			tmp = 0;
			offset_reg = (opcode & 0x0000f000) >> 12;
			if (offset_reg && (upeek(tcp, gpr_offset[offset_reg], &tmp) < 0))
				return -1;
			svc_addr += tmp;

			scno = ptrace(PTRACE_PEEKTEXT, tcp->pid, svc_addr, 0);
			if (errno)
				return -1;
#  if defined(S390X)
			scno >>= 48;
#  else
			scno >>= 16;
#  endif
			tmp = 0;
			offset_reg = (opcode & 0x00f00000) >> 20;
			if (offset_reg && (upeek(tcp, gpr_offset[offset_reg], &tmp) < 0))
				return -1;

			scno = (scno | tmp) & 0xff;
		}
	}
# elif defined (POWERPC)
	if (upeek(tcp, sizeof(unsigned long)*PT_R0, &scno) < 0)
		return -1;
	if (!(tcp->flags & TCB_INSYSCALL)) {
		/* Check if we return from execve. */
		if (scno == 0 && (tcp->flags & TCB_WAITEXECVE)) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}
	}
# elif defined(AVR32)
	/*
	 * Read complete register set in one go.
	 */
	if (ptrace(PTRACE_GETREGS, tcp->pid, NULL, &regs) < 0)
		return -1;

	/*
	 * We only need to grab the syscall number on syscall entry.
	 */
	if (!(tcp->flags & TCB_INSYSCALL)) {
		scno = regs.r8;

		/* Check if we return from execve. */
		if (tcp->flags & TCB_WAITEXECVE) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}
	}
# elif defined(BFIN)
	if (upeek(tcp, PT_ORIG_P0, &scno))
		return -1;
# elif defined (I386)
	if (upeek(tcp, 4*ORIG_EAX, &scno) < 0)
		return -1;
# elif defined (X86_64)
	if (upeek(tcp, 8*ORIG_RAX, &scno) < 0)
		return -1;

	if (!(tcp->flags & TCB_INSYSCALL)) {
		static int currpers = -1;
		long val;
		int pid = tcp->pid;

		/* Check CS register value. On x86-64 linux it is:
		 * 	0x33	for long mode (64 bit)
		 * 	0x23	for compatibility mode (32 bit)
		 * It takes only one ptrace and thus doesn't need
		 * to be cached.
		 */
		if (upeek(tcp, 8*CS, &val) < 0)
			return -1;
		switch (val) {
			case 0x23: currpers = 1; break;
			case 0x33: currpers = 0; break;
			default:
				fprintf(stderr, "Unknown value CS=0x%02X while "
					 "detecting personality of process "
					 "PID=%d\n", (int)val, pid);
				currpers = current_personality;
				break;
		}
#  if 0
		/* This version analyzes the opcode of a syscall instruction.
		 * (int 0x80 on i386 vs. syscall on x86-64)
		 * It works, but is too complicated.
		 */
		unsigned long val, rip, i;

		if (upeek(tcp, 8*RIP, &rip) < 0)
			perror("upeek(RIP)");

		/* sizeof(syscall) == sizeof(int 0x80) == 2 */
		rip -= 2;
		errno = 0;

		call = ptrace(PTRACE_PEEKTEXT, pid, (char *)rip, (char *)0);
		if (errno)
			printf("ptrace_peektext failed: %s\n",
					strerror(errno));
		switch (call & 0xffff) {
			/* x86-64: syscall = 0x0f 0x05 */
			case 0x050f: currpers = 0; break;
			/* i386: int 0x80 = 0xcd 0x80 */
			case 0x80cd: currpers = 1; break;
			default:
				currpers = current_personality;
				fprintf(stderr,
					"Unknown syscall opcode (0x%04X) while "
					"detecting personality of process "
					"PID=%d\n", (int)call, pid);
				break;
		}
#  endif
		if (currpers != current_personality) {
			static const char *const names[] = {"64 bit", "32 bit"};
			set_personality(currpers);
			printf("[ Process PID=%d runs in %s mode. ]\n",
					pid, names[current_personality]);
		}
	}
# elif defined(IA64)
#	define IA64_PSR_IS	((long)1 << 34)
	if (upeek (tcp, PT_CR_IPSR, &psr) >= 0)
		ia32 = (psr & IA64_PSR_IS) != 0;
	if (!(tcp->flags & TCB_INSYSCALL)) {
		if (ia32) {
			if (upeek(tcp, PT_R1, &scno) < 0)	/* orig eax */
				return -1;
		} else {
			if (upeek (tcp, PT_R15, &scno) < 0)
				return -1;
		}
		/* Check if we return from execve. */
		if (tcp->flags & TCB_WAITEXECVE) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}
	} else {
		/* syscall in progress */
		if (upeek (tcp, PT_R8, &r8) < 0)
			return -1;
		if (upeek (tcp, PT_R10, &r10) < 0)
			return -1;
	}
# elif defined (ARM)
	/*
	 * Read complete register set in one go.
	 */
	if (ptrace(PTRACE_GETREGS, tcp->pid, NULL, (void *)&regs) == -1)
		return -1;

	/*
	 * We only need to grab the syscall number on syscall entry.
	 */
	if (regs.ARM_ip == 0) {
		if (!(tcp->flags & TCB_INSYSCALL)) {
			/* Check if we return from execve. */
			if (tcp->flags & TCB_WAITEXECVE) {
				tcp->flags &= ~TCB_WAITEXECVE;
				return 0;
			}
		}

		/*
		 * Note: we only deal with only 32-bit CPUs here.
		 */
		if (regs.ARM_cpsr & 0x20) {
			/*
			 * Get the Thumb-mode system call number
			 */
			scno = regs.ARM_r7;
		} else {
			/*
			 * Get the ARM-mode system call number
			 */
			errno = 0;
			scno = ptrace(PTRACE_PEEKTEXT, tcp->pid, (void *)(regs.ARM_pc - 4), NULL);
			if (errno)
				return -1;

			if (scno == 0 && (tcp->flags & TCB_WAITEXECVE)) {
				tcp->flags &= ~TCB_WAITEXECVE;
				return 0;
			}

			/* Handle the EABI syscall convention.  We do not
			   bother converting structures between the two
			   ABIs, but basic functionality should work even
			   if strace and the traced program have different
			   ABIs.  */
			if (scno == 0xef000000) {
				scno = regs.ARM_r7;
			} else {
				if ((scno & 0x0ff00000) != 0x0f900000) {
					fprintf(stderr, "syscall: unknown syscall trap 0x%08lx\n",
						scno);
					return -1;
				}

				/*
				 * Fixup the syscall number
				 */
				scno &= 0x000fffff;
			}
		}
		if (scno & 0x0f0000) {
			/*
			 * Handle ARM specific syscall
			 */
			set_personality(1);
			scno &= 0x0000ffff;
		} else
			set_personality(0);

		if (tcp->flags & TCB_INSYSCALL) {
			fprintf(stderr, "pid %d stray syscall entry\n", tcp->pid);
			tcp->flags &= ~TCB_INSYSCALL;
		}
	} else {
		if (!(tcp->flags & TCB_INSYSCALL)) {
			fprintf(stderr, "pid %d stray syscall exit\n", tcp->pid);
			tcp->flags |= TCB_INSYSCALL;
		}
	}
# elif defined (M68K)
	if (upeek(tcp, 4*PT_ORIG_D0, &scno) < 0)
		return -1;
# elif defined (LINUX_MIPSN32)
	unsigned long long regs[38];

	if (ptrace (PTRACE_GETREGS, tcp->pid, NULL, (long) &regs) < 0)
		return -1;
	a3 = regs[REG_A3];
	r2 = regs[REG_V0];

	if(!(tcp->flags & TCB_INSYSCALL)) {
		scno = r2;

		/* Check if we return from execve. */
		if (scno == 0 && tcp->flags & TCB_WAITEXECVE) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}

		if (scno < 0 || scno > nsyscalls) {
			if(a3 == 0 || a3 == -1) {
				if(debug)
					fprintf (stderr, "stray syscall exit: v0 = %ld\n", scno);
				return 0;
			}
		}
	}
# elif defined (MIPS)
	if (upeek(tcp, REG_A3, &a3) < 0)
		return -1;
	if(!(tcp->flags & TCB_INSYSCALL)) {
		if (upeek(tcp, REG_V0, &scno) < 0)
			return -1;

		/* Check if we return from execve. */
		if (scno == 0 && tcp->flags & TCB_WAITEXECVE) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}

		if (scno < 0 || scno > nsyscalls) {
			if(a3 == 0 || a3 == -1) {
				if(debug)
					fprintf (stderr, "stray syscall exit: v0 = %ld\n", scno);
				return 0;
			}
		}
	} else {
		if (upeek(tcp, REG_V0, &r2) < 0)
			return -1;
	}
# elif defined (ALPHA)
	if (upeek(tcp, REG_A3, &a3) < 0)
		return -1;

	if (!(tcp->flags & TCB_INSYSCALL)) {
		if (upeek(tcp, REG_R0, &scno) < 0)
			return -1;

		/* Check if we return from execve. */
		if (scno == 0 && tcp->flags & TCB_WAITEXECVE) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}

		/*
		 * Do some sanity checks to figure out if it's
		 * really a syscall entry
		 */
		if (scno < 0 || scno > nsyscalls) {
			if (a3 == 0 || a3 == -1) {
				if (debug)
					fprintf (stderr, "stray syscall exit: r0 = %ld\n", scno);
				return 0;
			}
		}
	}
	else {
		if (upeek(tcp, REG_R0, &r0) < 0)
			return -1;
	}
# elif defined (SPARC) || defined (SPARC64)
	/* Everything we need is in the current register set. */
	if (ptrace(PTRACE_GETREGS, tcp->pid, (char *)&regs, 0) < 0)
		return -1;

	/* If we are entering, then disassemble the syscall trap. */
	if (!(tcp->flags & TCB_INSYSCALL)) {
		/* Retrieve the syscall trap instruction. */
		errno = 0;
#  if defined(SPARC64)
		trap = ptrace(PTRACE_PEEKTEXT, tcp->pid, (char *)regs.tpc, 0);
		trap >>= 32;
#  else
		trap = ptrace(PTRACE_PEEKTEXT, tcp->pid, (char *)regs.pc, 0);
#  endif
		if (errno)
			return -1;

		/* Disassemble the trap to see what personality to use. */
		switch (trap) {
		case 0x91d02010:
			/* Linux/SPARC syscall trap. */
			set_personality(0);
			break;
		case 0x91d0206d:
			/* Linux/SPARC64 syscall trap. */
			set_personality(2);
			break;
		case 0x91d02000:
			/* SunOS syscall trap. (pers 1) */
			fprintf(stderr,"syscall: SunOS no support\n");
			return -1;
		case 0x91d02008:
			/* Solaris 2.x syscall trap. (per 2) */
			set_personality(1);
			break;
		case 0x91d02009:
			/* NetBSD/FreeBSD syscall trap. */
			fprintf(stderr,"syscall: NetBSD/FreeBSD not supported\n");
			return -1;
		case 0x91d02027:
			/* Solaris 2.x gettimeofday */
			set_personality(1);
			break;
		default:
			/* Unknown syscall trap. */
			if(tcp->flags & TCB_WAITEXECVE) {
				tcp->flags &= ~TCB_WAITEXECVE;
				return 0;
			}
#  if defined (SPARC64)
			fprintf(stderr,"syscall: unknown syscall trap %08lx %016lx\n", trap, regs.tpc);
#  else
			fprintf(stderr,"syscall: unknown syscall trap %08lx %08lx\n", trap, regs.pc);
#  endif
			return -1;
		}

		/* Extract the system call number from the registers. */
		if (trap == 0x91d02027)
			scno = 156;
		else
			scno = regs.u_regs[U_REG_G1];
		if (scno == 0) {
			scno = regs.u_regs[U_REG_O0];
			memmove (&regs.u_regs[U_REG_O0], &regs.u_regs[U_REG_O1], 7*sizeof(regs.u_regs[0]));
		}
	}
# elif defined(HPPA)
	if (upeek(tcp, PT_GR20, &scno) < 0)
		return -1;
	if (!(tcp->flags & TCB_INSYSCALL)) {
		/* Check if we return from execve. */
		if ((tcp->flags & TCB_WAITEXECVE)) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}
	}
# elif defined(SH)
	/*
	 * In the new syscall ABI, the system call number is in R3.
	 */
	if (upeek(tcp, 4*(REG_REG0+3), &scno) < 0)
		return -1;

	if (scno < 0) {
		/* Odd as it may seem, a glibc bug has been known to cause
		   glibc to issue bogus negative syscall numbers.  So for
		   our purposes, make strace print what it *should* have been */
		long correct_scno = (scno & 0xff);
		if (debug)
			fprintf(stderr,
				"Detected glibc bug: bogus system call"
				" number = %ld, correcting to %ld\n",
				scno,
				correct_scno);
		scno = correct_scno;
	}

	if (!(tcp->flags & TCB_INSYSCALL)) {
		/* Check if we return from execve. */
		if (scno == 0 && tcp->flags & TCB_WAITEXECVE) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}
	}
# elif defined(SH64)
	if (upeek(tcp, REG_SYSCALL, &scno) < 0)
		return -1;
	scno &= 0xFFFF;

	if (!(tcp->flags & TCB_INSYSCALL)) {
		/* Check if we return from execve. */
		if (tcp->flags & TCB_WAITEXECVE) {
			tcp->flags &= ~TCB_WAITEXECVE;
			return 0;
		}
	}
# elif defined(CRISV10) || defined(CRISV32)
	if (upeek(tcp, 4*PT_R9, &scno) < 0)
		return -1;
# endif
#endif /* LINUX */

#ifdef SUNOS4
	if (upeek(tcp, uoff(u_arg[7]), &scno) < 0)
		return -1;
#elif defined(SH)
	/* new syscall ABI returns result in R0 */
	if (upeek(tcp, 4*REG_REG0, (long *)&r0) < 0)
		return -1;
#elif defined(SH64)
	/* ABI defines result returned in r9 */
	if (upeek(tcp, REG_GENERAL(9), (long *)&r9) < 0)
		return -1;
#endif

#ifdef USE_PROCFS
# ifdef HAVE_PR_SYSCALL
	scno = tcp->status.PR_SYSCALL;
# else
#  ifndef FREEBSD
	scno = tcp->status.PR_WHAT;
#  else
	if (pread(tcp->pfd_reg, &regs, sizeof(regs), 0) < 0) {
		perror("pread");
		return -1;
	}
	switch (regs.r_eax) {
	case SYS_syscall:
	case SYS___syscall:
		pread(tcp->pfd, &scno, sizeof(scno), regs.r_esp + sizeof(int));
		break;
	default:
		scno = regs.r_eax;
		break;
	}
#  endif /* FREEBSD */
# endif /* !HAVE_PR_SYSCALL */
#endif /* USE_PROCFS */

	if (!(tcp->flags & TCB_INSYSCALL))
		tcp->scno = scno;
	return 1;
}


long
known_scno(tcp)
struct tcb *tcp;
{
	long scno = tcp->scno;
	if (scno >= 0 && scno < nsyscalls && sysent[scno].native_scno != 0)
		scno = sysent[scno].native_scno;
	else
		scno += NR_SYSCALL_BASE;
	return scno;
}

/* Called in trace_syscall() at each syscall entry and exit.
 * Returns:
 * 0: "ignore this syscall", bail out of trace_syscall() silently.
 * 1: ok, continue in trace_syscall().
 * other: error, trace_syscall() should print error indicator
 *    ("????" etc) and bail out.
 */
static int
syscall_fixup(struct tcb *tcp)
{
#ifdef USE_PROCFS
	int scno = known_scno(tcp);

	if (!(tcp->flags & TCB_INSYSCALL)) {
		if (tcp->status.PR_WHY != PR_SYSENTRY) {
			if (
			    scno == SYS_fork
#ifdef SYS_vfork
			    || scno == SYS_vfork
#endif /* SYS_vfork */
#ifdef SYS_fork1
			    || scno == SYS_fork1
#endif /* SYS_fork1 */
#ifdef SYS_forkall
			    || scno == SYS_forkall
#endif /* SYS_forkall */
#ifdef SYS_rfork1
			    || scno == SYS_rfork1
#endif /* SYS_fork1 */
#ifdef SYS_rforkall
			    || scno == SYS_rforkall
#endif /* SYS_rforkall */
			    ) {
				/* We are returning in the child, fake it. */
				tcp->status.PR_WHY = PR_SYSENTRY;
				trace_syscall(tcp);
				tcp->status.PR_WHY = PR_SYSEXIT;
			}
			else {
				fprintf(stderr, "syscall: missing entry\n");
				tcp->flags |= TCB_INSYSCALL;
			}
		}
	}
	else {
		if (tcp->status.PR_WHY != PR_SYSEXIT) {
			fprintf(stderr, "syscall: missing exit\n");
			tcp->flags &= ~TCB_INSYSCALL;
		}
	}
#endif /* USE_PROCFS */
#ifdef SUNOS4
	if (!(tcp->flags & TCB_INSYSCALL)) {
		if (scno == 0) {
			fprintf(stderr, "syscall: missing entry\n");
			tcp->flags |= TCB_INSYSCALL;
		}
	}
	else {
		if (scno != 0) {
			if (debug) {
				/*
				 * This happens when a signal handler
				 * for a signal which interrupted a
				 * a system call makes another system call.
				 */
				fprintf(stderr, "syscall: missing exit\n");
			}
			tcp->flags &= ~TCB_INSYSCALL;
		}
	}
#endif /* SUNOS4 */
#ifdef LINUX
#if defined (I386)
	if (upeek(tcp, 4*EAX, &eax) < 0)
		return -1;
	if (eax != -ENOSYS && !(tcp->flags & TCB_INSYSCALL)) {
		if (debug)
			fprintf(stderr, "stray syscall exit: eax = %ld\n", eax);
		return 0;
	}
#elif defined (X86_64)
	if (upeek(tcp, 8*RAX, &rax) < 0)
		return -1;
	if (current_personality == 1)
		rax = (long int)(int)rax; /* sign extend from 32 bits */
	if (rax != -ENOSYS && !(tcp->flags & TCB_INSYSCALL)) {
		if (debug)
			fprintf(stderr, "stray syscall exit: rax = %ld\n", rax);
		return 0;
	}
#elif defined (S390) || defined (S390X)
	if (upeek(tcp, PT_GPR2, &gpr2) < 0)
		return -1;
	if (syscall_mode != -ENOSYS)
		syscall_mode = tcp->scno;
	if (gpr2 != syscall_mode && !(tcp->flags & TCB_INSYSCALL)) {
		if (debug)
			fprintf(stderr, "stray syscall exit: gpr2 = %ld\n", gpr2);
		return 0;
	}
	else if (((tcp->flags & (TCB_INSYSCALL|TCB_WAITEXECVE))
		  == (TCB_INSYSCALL|TCB_WAITEXECVE))
		 && (gpr2 == -ENOSYS || gpr2 == tcp->scno)) {
		/*
		 * Fake a return value of zero.  We leave the TCB_WAITEXECVE
		 * flag set for the post-execve SIGTRAP to see and reset.
		 */
		gpr2 = 0;
	}
#elif defined (POWERPC)
# define SO_MASK 0x10000000
	if (upeek(tcp, sizeof(unsigned long)*PT_CCR, &flags) < 0)
		return -1;
	if (upeek(tcp, sizeof(unsigned long)*PT_R3, &result) < 0)
		return -1;
	if (flags & SO_MASK)
		result = -result;
#elif defined (M68K)
	if (upeek(tcp, 4*PT_D0, &d0) < 0)
		return -1;
	if (d0 != -ENOSYS && !(tcp->flags & TCB_INSYSCALL)) {
		if (debug)
			fprintf(stderr, "stray syscall exit: d0 = %ld\n", d0);
		return 0;
	}
#elif defined (ARM)
	/*
	 * Nothing required
	 */
#elif defined(BFIN)
	if (upeek(tcp, PT_R0, &r0) < 0)
		return -1;
#elif defined (HPPA)
	if (upeek(tcp, PT_GR28, &r28) < 0)
		return -1;
#elif defined(IA64)
	if (upeek(tcp, PT_R10, &r10) < 0)
		return -1;
	if (upeek(tcp, PT_R8, &r8) < 0)
		return -1;
	if (ia32 && r8 != -ENOSYS && !(tcp->flags & TCB_INSYSCALL)) {
		if (debug)
			fprintf(stderr, "stray syscall exit: r8 = %ld\n", r8);
		return 0;
	}
#elif defined(CRISV10) || defined(CRISV32)
	if (upeek(tcp, 4*PT_R10, &r10) < 0)
		return -1;
	if (r10 != -ENOSYS && !(tcp->flags & TCB_INSYSCALL)) {
		if (debug)
			fprintf(stderr, "stray syscall exit: r10 = %ld\n", r10);
		return 0;
	}
#endif
#endif /* LINUX */
	return 1;
}

#ifdef LINUX
/*
 * Check the syscall return value register value for whether it is
 * a negated errno code indicating an error, or a success return value.
 */
static inline int
is_negated_errno(unsigned long int val)
{
	unsigned long int max = -(long int) nerrnos;
	if (personality_wordsize[current_personality] < sizeof(val)) {
		val = (unsigned int) val;
		max = (unsigned int) max;
	}
	return val > max;
}
#endif

static int
get_error(struct tcb *tcp)
{
	int u_error = 0;
#ifdef LINUX
# if defined(S390) || defined(S390X)
	if (is_negated_errno(gpr2)) {
		tcp->u_rval = -1;
		u_error = -gpr2;
	}
	else {
		tcp->u_rval = gpr2;
		u_error = 0;
	}
# elif defined(I386)
	if (is_negated_errno(eax)) {
		tcp->u_rval = -1;
		u_error = -eax;
	}
	else {
		tcp->u_rval = eax;
		u_error = 0;
	}
# elif defined(X86_64)
	if (is_negated_errno(rax)) {
		tcp->u_rval = -1;
		u_error = -rax;
	}
	else {
		tcp->u_rval = rax;
		u_error = 0;
	}
# elif defined(IA64)
	if (ia32) {
		int err;

		err = (int)r8;
		if (is_negated_errno(err)) {
			tcp->u_rval = -1;
			u_error = -err;
		}
		else {
			tcp->u_rval = err;
			u_error = 0;
		}
	} else {
		if (r10) {
			tcp->u_rval = -1;
			u_error = r8;
		} else {
			tcp->u_rval = r8;
			u_error = 0;
		}
	}
# elif defined(MIPS)
		if (a3) {
			tcp->u_rval = -1;
			u_error = r2;
		} else {
			tcp->u_rval = r2;
			u_error = 0;
		}
# elif defined(POWERPC)
		if (is_negated_errno(result)) {
			tcp->u_rval = -1;
			u_error = -result;
		}
		else {
			tcp->u_rval = result;
			u_error = 0;
		}
# elif defined(M68K)
		if (is_negated_errno(d0)) {
			tcp->u_rval = -1;
			u_error = -d0;
		}
		else {
			tcp->u_rval = d0;
			u_error = 0;
		}
# elif defined(ARM)
		if (is_negated_errno(regs.ARM_r0)) {
			tcp->u_rval = -1;
			u_error = -regs.ARM_r0;
		}
		else {
			tcp->u_rval = regs.ARM_r0;
			u_error = 0;
		}
# elif defined(AVR32)
		if (regs.r12 && (unsigned) -regs.r12 < nerrnos) {
			tcp->u_rval = -1;
			u_error = -regs.r12;
		}
		else {
			tcp->u_rval = regs.r12;
			u_error = 0;
		}
# elif defined(BFIN)
		if (is_negated_errno(r0)) {
			tcp->u_rval = -1;
			u_error = -r0;
		} else {
			tcp->u_rval = r0;
			u_error = 0;
		}
# elif defined(ALPHA)
		if (a3) {
			tcp->u_rval = -1;
			u_error = r0;
		}
		else {
			tcp->u_rval = r0;
			u_error = 0;
		}
# elif defined(SPARC)
		if (regs.psr & PSR_C) {
			tcp->u_rval = -1;
			u_error = regs.u_regs[U_REG_O0];
		}
		else {
			tcp->u_rval = regs.u_regs[U_REG_O0];
			u_error = 0;
		}
# elif defined(SPARC64)
		if (regs.tstate & 0x1100000000UL) {
			tcp->u_rval = -1;
			u_error = regs.u_regs[U_REG_O0];
		}
		else {
			tcp->u_rval = regs.u_regs[U_REG_O0];
			u_error = 0;
		}
# elif defined(HPPA)
		if (is_negated_errno(r28)) {
			tcp->u_rval = -1;
			u_error = -r28;
		}
		else {
			tcp->u_rval = r28;
			u_error = 0;
		}
# elif defined(SH)
		/* interpret R0 as return value or error number */
		if (is_negated_errno(r0)) {
			tcp->u_rval = -1;
			u_error = -r0;
		}
		else {
			tcp->u_rval = r0;
			u_error = 0;
		}
# elif defined(SH64)
		/* interpret result as return value or error number */
		if (is_negated_errno(r9)) {
			tcp->u_rval = -1;
			u_error = -r9;
		}
		else {
			tcp->u_rval = r9;
			u_error = 0;
		}
# elif defined(CRISV10) || defined(CRISV32)
		if (r10 && (unsigned) -r10 < nerrnos) {
			tcp->u_rval = -1;
			u_error = -r10;
		}
		else {
			tcp->u_rval = r10;
			u_error = 0;
		}
# endif
#endif /* LINUX */
#ifdef SUNOS4
		/* get error code from user struct */
		if (upeek(tcp, uoff(u_error), &u_error) < 0)
			return -1;
		u_error >>= 24; /* u_error is a char */

		/* get system call return value */
		if (upeek(tcp, uoff(u_rval1), &tcp->u_rval) < 0)
			return -1;
#endif /* SUNOS4 */
#ifdef SVR4
#ifdef SPARC
		/* Judicious guessing goes a long way. */
		if (tcp->status.pr_reg[R_PSR] & 0x100000) {
			tcp->u_rval = -1;
			u_error = tcp->status.pr_reg[R_O0];
		}
		else {
			tcp->u_rval = tcp->status.pr_reg[R_O0];
			u_error = 0;
		}
#endif /* SPARC */
#ifdef I386
		/* Wanna know how to kill an hour single-stepping? */
		if (tcp->status.PR_REG[EFL] & 0x1) {
			tcp->u_rval = -1;
			u_error = tcp->status.PR_REG[EAX];
		}
		else {
			tcp->u_rval = tcp->status.PR_REG[EAX];
#ifdef HAVE_LONG_LONG
			tcp->u_lrval =
				((unsigned long long) tcp->status.PR_REG[EDX] << 32) +
				tcp->status.PR_REG[EAX];
#endif
			u_error = 0;
		}
#endif /* I386 */
#ifdef X86_64
		/* Wanna know how to kill an hour single-stepping? */
		if (tcp->status.PR_REG[EFLAGS] & 0x1) {
			tcp->u_rval = -1;
			u_error = tcp->status.PR_REG[RAX];
		}
		else {
			tcp->u_rval = tcp->status.PR_REG[RAX];
			u_error = 0;
		}
#endif /* X86_64 */
#ifdef MIPS
		if (tcp->status.pr_reg[CTX_A3]) {
			tcp->u_rval = -1;
			u_error = tcp->status.pr_reg[CTX_V0];
		}
		else {
			tcp->u_rval = tcp->status.pr_reg[CTX_V0];
			u_error = 0;
		}
#endif /* MIPS */
#endif /* SVR4 */
#ifdef FREEBSD
		if (regs.r_eflags & PSL_C) {
			tcp->u_rval = -1;
		        u_error = regs.r_eax;
		} else {
			tcp->u_rval = regs.r_eax;
			tcp->u_lrval =
			  ((unsigned long long) regs.r_edx << 32) +  regs.r_eax;
		        u_error = 0;
		}
#endif /* FREEBSD */
	tcp->u_error = u_error;
	return 1;
}

int
force_result(tcp, error, rval)
	struct tcb *tcp;
	int error;
	long rval;
{
#ifdef LINUX
# if defined(S390) || defined(S390X)
	gpr2 = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)PT_GPR2, gpr2) < 0)
		return -1;
# elif defined(I386)
	eax = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(EAX * 4), eax) < 0)
		return -1;
# elif defined(X86_64)
	rax = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(RAX * 8), rax) < 0)
		return -1;
# elif defined(IA64)
	if (ia32) {
		r8 = error ? -error : rval;
		if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(PT_R8), r8) < 0)
			return -1;
	}
	else {
		if (error) {
			r8 = error;
			r10 = -1;
		}
		else {
			r8 = rval;
			r10 = 0;
		}
		if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(PT_R8), r8) < 0 ||
		    ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(PT_R10), r10) < 0)
			return -1;
	}
# elif defined(BFIN)
	r0 = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)PT_R0, r0) < 0)
		return -1;
# elif defined(MIPS)
	if (error) {
		r2 = error;
		a3 = -1;
	}
	else {
		r2 = rval;
		a3 = 0;
	}
	/* PTRACE_POKEUSER is OK even for n32 since rval is only a long.  */
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(REG_A3), a3) < 0 ||
	    ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(REG_V0), r2) < 0)
		return -1;
# elif defined(POWERPC)
	if (upeek(tcp, sizeof(unsigned long)*PT_CCR, &flags) < 0)
		return -1;
	if (error) {
		flags |= SO_MASK;
		result = error;
	}
	else {
		flags &= ~SO_MASK;
		result = rval;
	}
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(sizeof(unsigned long)*PT_CCR), flags) < 0 ||
	    ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(sizeof(unsigned long)*PT_R3), result) < 0)
		return -1;
# elif defined(M68K)
	d0 = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(4*PT_D0), d0) < 0)
		return -1;
# elif defined(ARM)
	regs.ARM_r0 = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(4*0), regs.ARM_r0) < 0)
		return -1;
# elif defined(AVR32)
	regs.r12 = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)REG_R12, regs.r12) < 0)
		return -1;
# elif defined(ALPHA)
	if (error) {
		a3 = -1;
		r0 = error;
	}
	else {
		a3 = 0;
		r0 = rval;
	}
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(REG_A3), a3) < 0 ||
	    ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(REG_R0), r0) < 0)
		return -1;
# elif defined(SPARC)
	if (ptrace(PTRACE_GETREGS, tcp->pid, (char *)&regs, 0) < 0)
		return -1;
	if (error) {
		regs.psr |= PSR_C;
		regs.u_regs[U_REG_O0] = error;
	}
	else {
		regs.psr &= ~PSR_C;
		regs.u_regs[U_REG_O0] = rval;
	}
	if (ptrace(PTRACE_SETREGS, tcp->pid, (char *)&regs, 0) < 0)
		return -1;
# elif defined(SPARC64)
	if (ptrace(PTRACE_GETREGS, tcp->pid, (char *)&regs, 0) < 0)
		return -1;
	if (error) {
		regs.tstate |= 0x1100000000UL;
		regs.u_regs[U_REG_O0] = error;
	}
	else {
		regs.tstate &= ~0x1100000000UL;
		regs.u_regs[U_REG_O0] = rval;
	}
	if (ptrace(PTRACE_SETREGS, tcp->pid, (char *)&regs, 0) < 0)
		return -1;
# elif defined(HPPA)
	r28 = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(PT_GR28), r28) < 0)
		return -1;
# elif defined(SH)
	r0 = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)(4*REG_REG0), r0) < 0)
		return -1;
# elif defined(SH64)
	r9 = error ? -error : rval;
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)REG_GENERAL(9), r9) < 0)
		return -1;
# endif
#endif /* LINUX */

#ifdef SUNOS4
	if (ptrace(PTRACE_POKEUSER, tcp->pid, (char*)uoff(u_error),
		   error << 24) < 0 ||
	    ptrace(PTRACE_POKEUSER, tcp->pid, (char*)uoff(u_rval1), rval) < 0)
		return -1;
#endif /* SUNOS4 */

#ifdef SVR4
	/* XXX no clue */
	return -1;
#endif /* SVR4 */

#ifdef FREEBSD
	if (pread(tcp->pfd_reg, &regs, sizeof(regs), 0) < 0) {
		perror("pread");
		return -1;
	}
	if (error) {
		regs.r_eflags |= PSL_C;
		regs.r_eax = error;
	}
	else {
		regs.r_eflags &= ~PSL_C;
		regs.r_eax = rval;
	}
	if (pwrite(tcp->pfd_reg, &regs, sizeof(regs), 0) < 0) {
		perror("pwrite");
		return -1;
	}
#endif /* FREEBSD */

	/* All branches reach here on success (only).  */
	tcp->u_error = error;
	tcp->u_rval = rval;
	return 0;
}

static int
syscall_enter(struct tcb *tcp)
{
#ifdef LINUX
#if defined(S390) || defined(S390X)
	{
		int i;
		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++) {
			if (upeek(tcp,i==0 ? PT_ORIGGPR2:PT_GPR2+i*sizeof(long), &tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#elif defined (ALPHA)
	{
		int i;
		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++) {
			/* WTA: if scno is out-of-bounds this will bomb. Add range-check
			 * for scno somewhere above here!
			 */
			if (upeek(tcp, REG_A0+i, &tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#elif defined (IA64)
	{
		if (!ia32) {
			unsigned long *out0, cfm, sof, sol, i;
			long rbs_end;
			/* be backwards compatible with kernel < 2.4.4... */
#			ifndef PT_RBS_END
#			  define PT_RBS_END	PT_AR_BSP
#			endif

			if (upeek(tcp, PT_RBS_END, &rbs_end) < 0)
				return -1;
			if (upeek(tcp, PT_CFM, (long *) &cfm) < 0)
				return -1;

			sof = (cfm >> 0) & 0x7f;
			sol = (cfm >> 7) & 0x7f;
			out0 = ia64_rse_skip_regs((unsigned long *) rbs_end, -sof + sol);

			if (tcp->scno >= 0 && tcp->scno < nsyscalls
			    && sysent[tcp->scno].nargs != -1)
				tcp->u_nargs = sysent[tcp->scno].nargs;
			else
				tcp->u_nargs = MAX_ARGS;
			for (i = 0; i < tcp->u_nargs; ++i) {
				if (umoven(tcp, (unsigned long) ia64_rse_skip_regs(out0, i),
					   sizeof(long), (char *) &tcp->u_arg[i]) < 0)
					return -1;
			}
		} else {
			int i;

			if (/* EBX = out0 */
			    upeek(tcp, PT_R11, (long *) &tcp->u_arg[0]) < 0
			    /* ECX = out1 */
			    || upeek(tcp, PT_R9,  (long *) &tcp->u_arg[1]) < 0
			    /* EDX = out2 */
			    || upeek(tcp, PT_R10, (long *) &tcp->u_arg[2]) < 0
			    /* ESI = out3 */
			    || upeek(tcp, PT_R14, (long *) &tcp->u_arg[3]) < 0
			    /* EDI = out4 */
			    || upeek(tcp, PT_R15, (long *) &tcp->u_arg[4]) < 0
			    /* EBP = out5 */
			    || upeek(tcp, PT_R13, (long *) &tcp->u_arg[5]) < 0)
				return -1;

			for (i = 0; i < 6; ++i)
				/* truncate away IVE sign-extension */
				tcp->u_arg[i] &= 0xffffffff;

			if (tcp->scno >= 0 && tcp->scno < nsyscalls
			    && sysent[tcp->scno].nargs != -1)
				tcp->u_nargs = sysent[tcp->scno].nargs;
			else
				tcp->u_nargs = 5;
		}
	}
#elif defined (LINUX_MIPSN32) || defined (LINUX_MIPSN64)
	/* N32 and N64 both use up to six registers.  */
	{
		unsigned long long regs[38];
		int i, nargs;

		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			nargs = tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			nargs = tcp->u_nargs = MAX_ARGS;

		if (ptrace (PTRACE_GETREGS, pid, NULL, (long) &regs) < 0)
			return -1;

		for(i = 0; i < nargs; i++) {
			tcp->u_arg[i] = regs[REG_A0 + i];
# if defined (LINUX_MIPSN32)
			tcp->ext_arg[i] = regs[REG_A0 + i];
# endif
		}
	}
#elif defined (MIPS)
	{
		long sp;
		int i, nargs;

		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			nargs = tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			nargs = tcp->u_nargs = MAX_ARGS;
		if(nargs > 4) {
			if(upeek(tcp, REG_SP, &sp) < 0)
				return -1;
			for(i = 0; i < 4; i++) {
				if (upeek(tcp, REG_A0 + i, &tcp->u_arg[i])<0)
					return -1;
			}
			umoven(tcp, sp+16, (nargs-4) * sizeof(tcp->u_arg[0]),
			       (char *)(tcp->u_arg + 4));
		} else {
			for(i = 0; i < nargs; i++) {
				if (upeek(tcp, REG_A0 + i, &tcp->u_arg[i]) < 0)
					return -1;
			}
		}
	}
#elif defined (POWERPC)
# ifndef PT_ORIG_R3
#  define PT_ORIG_R3 34
# endif
	{
		int i;
		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++) {
			if (upeek(tcp, (i==0) ?
				(sizeof(unsigned long)*PT_ORIG_R3) :
				((i+PT_R3)*sizeof(unsigned long)),
					&tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#elif defined (SPARC) || defined (SPARC64)
	{
		int i;

		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++)
			tcp->u_arg[i] = regs.u_regs[U_REG_O0 + i];
	}
#elif defined (HPPA)
	{
		int i;

		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++) {
			if (upeek(tcp, PT_GR26-4*i, &tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#elif defined(ARM)
	{
		int i;

		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++)
			tcp->u_arg[i] = regs.uregs[i];
	}
#elif defined(AVR32)
	tcp->u_nargs = sysent[tcp->scno].nargs;
	tcp->u_arg[0] = regs.r12;
	tcp->u_arg[1] = regs.r11;
	tcp->u_arg[2] = regs.r10;
	tcp->u_arg[3] = regs.r9;
	tcp->u_arg[4] = regs.r5;
	tcp->u_arg[5] = regs.r3;
#elif defined(BFIN)
	{
		int i;
		int argreg[] = {PT_R0, PT_R1, PT_R2, PT_R3, PT_R4, PT_R5};

		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = sizeof(argreg) / sizeof(argreg[0]);

		for (i = 0; i < tcp->u_nargs; ++i)
			if (upeek(tcp, argreg[i], &tcp->u_arg[i]) < 0)
				return -1;
	}
#elif defined(SH)
	{
		int i;
		static int syscall_regs[] = {
			REG_REG0+4, REG_REG0+5, REG_REG0+6, REG_REG0+7,
			REG_REG0, REG_REG0+1, REG_REG0+2
		};

		tcp->u_nargs = sysent[tcp->scno].nargs;
		for (i = 0; i < tcp->u_nargs; i++) {
			if (upeek(tcp, 4*syscall_regs[i], &tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#elif defined(SH64)
	{
		int i;
		/* Registers used by SH5 Linux system calls for parameters */
		static int syscall_regs[] = { 2, 3, 4, 5, 6, 7 };

		/*
		 * TODO: should also check that the number of arguments encoded
		 *       in the trap number matches the number strace expects.
		 */
		/*
		assert(sysent[tcp->scno].nargs <
		       sizeof(syscall_regs)/sizeof(syscall_regs[0]));
		 */

		tcp->u_nargs = sysent[tcp->scno].nargs;
		for (i = 0; i < tcp->u_nargs; i++) {
			if (upeek(tcp, REG_GENERAL(syscall_regs[i]), &tcp->u_arg[i]) < 0)
				return -1;
		}
	}

#elif defined(X86_64)
	{
		int i;
		static int argreg[SUPPORTED_PERSONALITIES][MAX_ARGS] = {
			{RDI,RSI,RDX,R10,R8,R9},	/* x86-64 ABI */
			{RBX,RCX,RDX,RSI,RDI,RBP}	/* i386 ABI */
		};

		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++) {
			if (upeek(tcp, argreg[current_personality][i]*8, &tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#elif defined(CRISV10) || defined(CRISV32)
	{
		int i;
		static const int crisregs[] = {
			4*PT_ORIG_R10, 4*PT_R11, 4*PT_R12,
			4*PT_R13, 4*PT_MOF, 4*PT_SRP
		};

		if (tcp->scno >= 0 && tcp->scno < nsyscalls)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = 0;
		for (i = 0; i < tcp->u_nargs; i++) {
			if (upeek(tcp, crisregs[i], &tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#else /* Other architecture (like i386) (32bits specific) */
	{
		int i;
		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++) {
			if (upeek(tcp, i*4, &tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#endif
#endif /* LINUX */
#ifdef SUNOS4
	{
		int i;
		if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
			tcp->u_nargs = sysent[tcp->scno].nargs;
		else
			tcp->u_nargs = MAX_ARGS;
		for (i = 0; i < tcp->u_nargs; i++) {
			struct user *u;

			if (upeek(tcp, uoff(u_arg[0]) +
			    (i*sizeof(u->u_arg[0])), &tcp->u_arg[i]) < 0)
				return -1;
		}
	}
#endif /* SUNOS4 */
#ifdef SVR4
#ifdef MIPS
	/*
	 * SGI is broken: even though it has pr_sysarg, it doesn't
	 * set them on system call entry.  Get a clue.
	 */
	if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
		tcp->u_nargs = sysent[tcp->scno].nargs;
	else
		tcp->u_nargs = tcp->status.pr_nsysarg;
	if (tcp->u_nargs > 4) {
		memcpy(tcp->u_arg, &tcp->status.pr_reg[CTX_A0],
			4*sizeof(tcp->u_arg[0]));
		umoven(tcp, tcp->status.pr_reg[CTX_SP] + 16,
			(tcp->u_nargs - 4)*sizeof(tcp->u_arg[0]), (char *) (tcp->u_arg + 4));
	}
	else {
		memcpy(tcp->u_arg, &tcp->status.pr_reg[CTX_A0],
			tcp->u_nargs*sizeof(tcp->u_arg[0]));
	}
#elif UNIXWARE >= 2
	/*
	 * Like SGI, UnixWare doesn't set pr_sysarg until system call exit
	 */
	if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
		tcp->u_nargs = sysent[tcp->scno].nargs;
	else
		tcp->u_nargs = tcp->status.pr_lwp.pr_nsysarg;
	umoven(tcp, tcp->status.PR_REG[UESP] + 4,
		tcp->u_nargs*sizeof(tcp->u_arg[0]), (char *) tcp->u_arg);
#elif defined (HAVE_PR_SYSCALL)
	if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
		tcp->u_nargs = sysent[tcp->scno].nargs;
	else
		tcp->u_nargs = tcp->status.pr_nsysarg;
	{
		int i;
		for (i = 0; i < tcp->u_nargs; i++)
			tcp->u_arg[i] = tcp->status.pr_sysarg[i];
	}
#elif defined (I386)
	if (tcp->scno >= 0 && tcp->scno < nsyscalls && sysent[tcp->scno].nargs != -1)
		tcp->u_nargs = sysent[tcp->scno].nargs;
	else
		tcp->u_nargs = 5;
	umoven(tcp, tcp->status.PR_REG[UESP] + 4,
		tcp->u_nargs*sizeof(tcp->u_arg[0]), (char *) tcp->u_arg);
#else
	I DONT KNOW WHAT TO DO
#endif /* !HAVE_PR_SYSCALL */
#endif /* SVR4 */
#ifdef FREEBSD
	if (tcp->scno >= 0 && tcp->scno < nsyscalls &&
	    sysent[tcp->scno].nargs > tcp->status.val)
		tcp->u_nargs = sysent[tcp->scno].nargs;
	else
		tcp->u_nargs = tcp->status.val;
	if (tcp->u_nargs < 0)
		tcp->u_nargs = 0;
	if (tcp->u_nargs > MAX_ARGS)
		tcp->u_nargs = MAX_ARGS;
	switch(regs.r_eax) {
	case SYS___syscall:
		pread(tcp->pfd, &tcp->u_arg, tcp->u_nargs * sizeof(unsigned long),
		      regs.r_esp + sizeof(int) + sizeof(quad_t));
		break;
	case SYS_syscall:
		pread(tcp->pfd, &tcp->u_arg, tcp->u_nargs * sizeof(unsigned long),
		      regs.r_esp + 2 * sizeof(int));
		break;
	default:
		pread(tcp->pfd, &tcp->u_arg, tcp->u_nargs * sizeof(unsigned long),
		      regs.r_esp + sizeof(int));
		break;
	}
#endif /* FREEBSD */
	return 1;
}

enum { record_debug = 0 };
enum { namei_debug = 0 };

static void
record_namei(syscall_record *rec, struct tcb *tcp, int argn, int cwd_fd)
{
    rec->args.args_val[argn].namei = malloc(sizeof(struct namei_infos));
    rec->args.args_val[argn].namei->ni.ni_len = 0;
    rec->args.args_val[argn].namei->ni.ni_val = 0;

    char pn[1024] = {0};
    umovestr(tcp, tcp->u_arg[argn], sizeof(pn), pn);

    if (namei_debug)
	fprintf(stderr, "record_namei: %s\n", pn);

    int fd;
    char buf[1024];
    if (pn[0] == '/') {
	sprintf(&buf[0], "/proc/%d/root", tcp->pid);
    } else {
	if (cwd_fd == AT_FDCWD)
	    sprintf(&buf[0], "/proc/%d/cwd", tcp->pid);
	else
	    sprintf(&buf[0], "/proc/%d/fd/%d", tcp->pid, cwd_fd);
    }
    fd = open(buf, O_RDONLY);

    char *p = &pn[0];
    while (fd >= 0) {
	rec->args.args_val[argn].namei->ni.ni_len++;
	rec->args.args_val[argn].namei->ni.ni_val =
	   realloc(rec->args.args_val[argn].namei->ni.ni_val,
		   rec->args.args_val[argn].namei->ni.ni_len * sizeof(namei_info));
	namei_info *ni = &rec->args.args_val[argn].namei->ni.ni_val[rec->args.args_val[argn].namei->ni.ni_len - 1];

	while (*p == '/')
	    p++;

	char *np = strchr(p, '/');
	if (np)
	    *np = '\0';

	struct stat st;
	fstat(fd, &st);
	ni->dev = st.st_dev;
	ni->ino = st.st_ino;
	ni->gen = 0;
	ni->name = strdup(p);
	ioctl(fd, FS_IOC_GETVERSION, &ni->gen);

	int nfd = openat(fd, p, O_RDONLY);
	close(fd);
	fd = nfd;
	p = np ? np + 1 : p + strlen(p);

	if (namei_debug)
	    fprintf(stderr, "record_namei: %ld %ld %s (%d)\n",
		    ni->dev, ni->ino, ni->name, fd);
    }
}

static int
record_path_stat(struct stat_info *sp, const char *path, struct tcb *tcp, int tfd)
{
    struct stat st = { 0 };
    int gen = 0;

    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
	if (fstat(fd, &st) < 0) {
	    fprintf(stderr, "record_path_stat: cannot fstat %s: %s\n",
		    path, strerror(errno));
	    close(fd);
	    return -1;
	}

	ioctl(fd, FS_IOC_GETVERSION, &gen);
	close(fd);
    } else {
	char pnbuf[1024];
	int cc = readlink(path, pnbuf, sizeof(pnbuf) - 1);

	if (cc < 0) cc = 0;
	pnbuf[cc] = '\0';

	if (!strncmp(pnbuf, "socket:[", 8)) {
	    st.st_dev = 6;
	    st.st_ino = atoi(&pnbuf[8]);
	} else if (!strncmp(pnbuf, "pipe:[", 6)) {
	    st.st_dev = 8;
	    st.st_ino = atoi(&pnbuf[6]);
	} else {
#if 0
	    fprintf(stderr, "record_path_stat: cannot open %s: %s (readlink %s)\n",
		    path, strerror(errno), pnbuf);
#endif
	    return -1;
	}
    }

    sp->dev = st.st_dev;
    sp->ino = st.st_ino;
    sp->gen = gen;
    sp->mode = st.st_mode;
    sp->rdev = st.st_rdev;

    if (S_ISCHR(st.st_mode) &&
	st.st_rdev == makedev(5, 2) &&	    /* /dev/ptmx */
	tfd >= 0 &&
	!entering(tcp))
    {
	struct user_regs_struct regs_save, regs_new;
	long stack_save, stack_new = 0;

	/* save old registers, and a word on the stack */
	if (ptrace(PTRACE_GETREGS, tcp->pid, 0, &regs_save) < 0)
	    perror("PTRACE_GETREGS 0");
	stack_save = ptrace(PTRACE_PEEKDATA, tcp->pid, regs_save.rsp, 0);
	if (errno)
	    perror("PTRACE_PEEKDATA 0");

	/* set up a set of registers to do a system call */
	regs_new = regs_save;
	regs_new.rip -= 2;
	regs_new.rax = __NR_ioctl;
	regs_new.rdi = tfd;
	regs_new.rsi = TIOCGPTN;
	regs_new.rdx = regs_save.rsp;

	if (ptrace(PTRACE_SETREGS, tcp->pid, 0, &regs_new) < 0)
	    perror("PTRACE_SETREGS 1");

	for (int i = 0; i < 2; i++) {
	    /* issue a system call */
	    if (ptrace(PTRACE_SINGLESTEP, tcp->pid, 0, 0) < 0)
		perror("PTRACE_SINGLESTEP 2");

	    int status;
	    if (waitpid(tcp->pid, &status, 0) < 0)
		perror("waitpid 3");

	    /* get the regs and the stack value */
	    if (ptrace(PTRACE_GETREGS, tcp->pid, 0, &regs_new) < 0)
		perror("PTRACE_GETREGS 3");

	    if (i == 1) {
		stack_new = ptrace(PTRACE_PEEKDATA, tcp->pid, regs_save.rsp, 0);
		if (errno)
		    perror("PTRACE_PEEKDATA 3");

		if (ptrace(PTRACE_POKEDATA, tcp->pid, regs_save.rsp, stack_save) < 0)
		    perror("PTRACE_POKEDATA 4");
		if (ptrace(PTRACE_SETREGS, tcp->pid, 0, &regs_save) < 0)
		    perror("PTRACE_SETREGS 4");
	    }
	}

	sp->ptsid = (int) stack_new;
    }
    return 0;
}

static void
record_path_stat_a(struct stat_info **pp, const char *path, struct tcb *tcp, int tfd)
{
    (*pp) = calloc(1, sizeof(struct stat_info));
    int r = record_path_stat(*pp, path, tcp, tfd);
    if (r < 0) {
	free(*pp);
	(*pp) = 0;
    }
}

static int
record_fd_stat(struct stat_info *sp, struct tcb *tcp, int fdno)
{
    char path[256];
    sprintf(&path[0], "/proc/%d/fd/%d", tcp->pid, fdno);
    int r = record_path_stat(sp, path, tcp, fdno);
    if (r < 0)
	return r;

    sprintf(&path[0], "/proc/%d/fdinfo/%d", tcp->pid, fdno);
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
	char buf[256];
	int cc = read(fd, &buf[0], sizeof(buf));
	if (cc >= 0) buf[cc] = '\0';
	close(fd);

	char *p = strstr(buf, "pos:");
	if (p) {
	    p += 4;
	    while (*p && (*p == ' ' || *p == '\t')) p++;
	    sp->fd_offset = strtol(p, 0, 10);
	}

	p = strstr(buf, "flags:");
	if (p) {
	    p += 6;
	    while (*p && (*p == ' ' || *p == '\t')) p++;
	    sp->fd_flags = strtol(p, 0, 8);
	}
    }
    return 0;
}

static void
record_fd_stat_a(struct stat_info **pp, struct tcb *tcp, int fdno)
{
    (*pp) = calloc(1, sizeof(struct stat_info));
    int r = record_fd_stat(*pp, tcp, fdno);
    if (r < 0) {
	free(*pp);
	(*pp) = 0;
    }
}

static void
record_ret_fd_stat(syscall_record *rec, struct tcb *tcp)
{
    int retfd = tcp->u_rval;
    if (retfd < 0)
	return;

    record_fd_stat_a(&rec->ret_fd_stat, tcp, retfd);
}

static void
record_arg_fd_stat(syscall_record *rec, struct tcb *tcp, int argn)
{
    int fd = tcp->u_arg[argn];
    if (fd == AT_FDCWD) {
	char path[256];
	sprintf(&path[0], "/proc/%d/cwd", tcp->pid);
	record_path_stat_a(&rec->args.args_val[argn].fd_stat, path, tcp, -1);
    } else {
	record_fd_stat_a(&rec->args.args_val[argn].fd_stat, tcp, fd);
    }
}

static void
record_exec(syscall_record *rec, struct tcb *tcp)
{
    rec->execi = calloc(sizeof(*rec->execi), 1);

    /* XXX determine how many FDs the process really had open at exec time */
    rec->execi->fds.fds_len = 3;
    rec->execi->fds.fds_val = calloc(sizeof(*rec->execi->fds.fds_val), rec->execi->fds.fds_len);

    char cwdbuf[256];
    snprintf(&cwdbuf[0], sizeof(cwdbuf), "/proc/%d/cwd", tcp->pid);
    record_path_stat(&rec->execi->cwd, cwdbuf, tcp, -1);

    char rootbuf[256];
    snprintf(rootbuf, sizeof(rootbuf), "/proc/%d/root", tcp->pid);
    record_path_stat(&rec->execi->root, rootbuf, tcp, -1);

    for (int i = 0; i < rec->execi->fds.fds_len; i++)
	record_fd_stat(&rec->execi->fds.fds_val[i], tcp, i);
}

static void
wait_for_command(struct tcb *tcp);

// move the filename given by the i-th argument into buffer `path' sized `len'
static void
umovepath(char *path, size_t len, struct tcb *tcp, size_t i, int cwd_fd)
{
    char buf[1024];
    umovestr(tcp, tcp->u_arg[i], sizeof(buf), buf);
    if (buf[0] == '/')
	snprintf(path, len, "/proc/%d/root%s", tcp->pid, buf);
    else {
	if (cwd_fd == AT_FDCWD)
	    snprintf(path, len, "/proc/%d/cwd/%s", tcp->pid, buf);
	else
	    snprintf(path, len, "/proc/%d/fd/%d/%s", tcp->pid, cwd_fd, buf);
    }
}

// record directory entries in the parent node of `path'
// note `path' will be either added or removed from its parent
static void
record_snap_dirents(syscall_record *rec, const char *path)
{
#define SNAP_DIRENTS_OPT 0
    // parent path
    DIR *dirp;
    {
	char parent[PATH_MAX + 1];
	if (!realpath(path, parent))
	    return;
	assert(parent[0] == '/');
	assert(parent[1]);
	for (char *p = parent + strlen(parent); *p != '/' && p > parent; --p)
	    *p = 0;
	dirp = opendir(parent);
	if (!dirp)
	    return;
    }

    int pfd = dirfd(dirp);
    struct stat st = {0};
    if (fstat(pfd, &st))
	err(1, "fstat");

    dirent_infos *di;
#if SNAP_DIRENTS_OPT
    // try to reuse
    di = rec->snap.dirents.dirents_val;
    for (int i = 0; i < rec->snap.dirents.dirents_len; ++i, ++di) {
	if (di->dev == st.st_dev && di->ino == st.st_ino)
	    break;
    }
    // init
    if (di == rec->snap.dirents.dirents_val + rec->snap.dirents.dirents_len) {
#endif
    ++rec->snap.dirents.dirents_len;
    rec->snap.dirents.dirents_val = realloc(rec->snap.dirents.dirents_val, sizeof(dirent_infos) * rec->snap.dirents.dirents_len);
    di = &rec->snap.dirents.dirents_val[rec->snap.dirents.dirents_len - 1];
    bzero(di, sizeof(dirent_infos));
    di->dev = st.st_dev;
    di->ino = st.st_ino;
    ioctl(pfd, FS_IOC_GETVERSION, &di->gen);
#if SNAP_DIRENTS_OPT
    }

    struct stat subst = {0};
    if (stat(path, &subst)) {
	closedir(dirp);
	return;
    }
#endif
    // loop for each entry in the parent directory
    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL) {
#if SNAP_DIRENTS_OPT
	if (subst.st_ino != dp->d_ino)
	    continue;
#endif
	++di->ni.ni_len;
	di->ni.ni_val = realloc(di->ni.ni_val, di->ni.ni_len * sizeof(namei_info));
	struct namei_info *n = &di->ni.ni_val[di->ni.ni_len - 1];
	n->name = strdup(dp->d_name);
	n->dev = st.st_dev;
	n->ino = dp->d_ino;
	n->gen = 0;
	int fd = openat(pfd, n->name, O_RDONLY | O_NOFOLLOW);
	if (fd >= 0) {
	    ioctl(fd, FS_IOC_GETVERSION, &n->gen);
	    close(fd);
	}
#if SNAP_DIRENTS_OPT
	break;
#endif
    }
    closedir(dirp);
#undef SNAP_DIRENTS_OPT
}

// make a hard link to path so as to keep a reference
static void
linkpath(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
	return;
    struct stat st = {0};
    if (fstat(fd, &st))
	err(1, "fstat");
    long gen = 0;
    ioctl(fd, FS_IOC_GETVERSION, &gen);
    close(fd);

    char lpath[1024];
    sprintf(lpath, "/mnt/undofs/snap/%ld_%ld_%ld", st.st_dev, st.st_ino, gen);
    if (link(path, lpath) && errno != EEXIST)
	perror("link");
}

static void
record_syscall(struct tcb *tcp)
{
    static int xdrs_init;
    static XDR xdrs;

    if (!xdrs_init) {
	setvbuf(tcp->outf, 0, _IONBF, 0);
	xdrstdio_create(&xdrs, tcp->outf, XDR_ENCODE);
	xdrs_init = 1;
    }

    const char *sys_name;
    if (tcp->scno >= 0 && tcp->scno < nsyscalls) {
	sys_name = sysent[tcp->scno].sys_name;
    } else if (tcp->scno >= SYS_undo_base && tcp->scno < SYS_undo_max) {
	switch (tcp->scno) {
	case SYS_undo_func_start:
	    tcp->u_nargs = 4; sys_name = "undo_func_start"; break;
	case SYS_undo_func_end:
	    tcp->u_nargs = 2; sys_name = "undo_func_end";   break;
	case SYS_undo_mask_start:
	    tcp->u_nargs = 1; sys_name = "undo_mask_start"; break;
	case SYS_undo_mask_end:
	    tcp->u_nargs = 1; sys_name = "undo_mask_end";   break;
	case SYS_undo_depend:
	    tcp->u_nargs = 5; sys_name = "undo_depend";	    break;
	}
    } else {
	if (tcp->scno != 0xc0ffee)	/* Magic value used by rerun */
	    fprintf(stderr, "Unknown syscall %ld\n", tcp->scno);
	tcp->u_nargs = 0;
	sys_name = "unknown";
    }

    const struct sysarg_call *sa = &sysarg_calls[0];
    while (sa->name) {
	if (!strcmp(sys_name, sa->name))
	    break;
	sa++;
    }

    if (!sa->name && tcp->u_nargs > 0) {
	fprintf(stderr, "Could not find sysargs for %ld [%s]\n",
		tcp->scno, sys_name);
	return;
    }

    syscall_record rec;
    memset(&rec, 0, sizeof(rec));

    rec.pid = tcp->pid;
    rec.scno = tcp->scno;
    rec.scname = (char *) sys_name;
    rec.enter = entering(tcp);
    rec.ret = tcp->u_rval;
    rec.err = tcp->u_error;

    rec.args.args_len = tcp->u_nargs;
    rec.args.args_val = calloc(sizeof(syscall_arg), rec.args.args_len);

    switch (tcp->scno) {
    case SYS_open:
    case SYS_creat:
    case SYS_execve:
    case SYS_unlink:
    case SYS_mkdir:
    case SYS_rmdir:
	record_namei(&rec, tcp, 0, AT_FDCWD);
	break;

    case SYS_link:
    case SYS_symlink:
    case SYS_rename:
	record_namei(&rec, tcp, 0, AT_FDCWD);
	record_namei(&rec, tcp, 1, AT_FDCWD);
	break;

    case SYS_utimensat:
    case SYS_futimesat:
    case SYS_newfstatat:
    case SYS_readlinkat:
    case SYS_faccessat:
    case SYS_fchmodat:
    case SYS_fchownat:
    case SYS_openat:
    case SYS_mknodat:
    case SYS_mkdirat:
    case SYS_unlinkat:
	record_arg_fd_stat(&rec, tcp, 0);
	record_namei(&rec, tcp, 1, tcp->u_arg[0]);
	break;

    case SYS_symlinkat:
	record_arg_fd_stat(&rec, tcp, 1);
	record_namei(&rec, tcp, 2, tcp->u_arg[1]);
	break;

    case SYS_linkat:
    case SYS_renameat:
	record_arg_fd_stat(&rec, tcp, 0);
	record_namei(&rec, tcp, 1, tcp->u_arg[0]);
	record_arg_fd_stat(&rec, tcp, 2);
	record_namei(&rec, tcp, 3, tcp->u_arg[2]);
	break;

    case SYS_fstat:
    case SYS_read:
    case SYS_pread64:
    case SYS_readv:
    case SYS_write:
    case SYS_pwrite64:
    case SYS_writev:

    case SYS_undo_mask_start:
    case SYS_undo_mask_end:
    case SYS_undo_depend:

    //case SYS_preadv:
    //case SYS_pwritev:
	record_arg_fd_stat(&rec, tcp, 0);
	break;
    }

    if (!entering(tcp)) {
	switch (tcp->scno) {
	case SYS_open:
	case SYS_creat:
	case SYS_openat:
	    record_ret_fd_stat(&rec, tcp);
	    break;
	}
    }

    if (tcp->scno == SYS_execve && entering(tcp))
	record_exec(&rec, tcp);

    if (record_debug)
	fprintf(stderr, "syscall: pid %d, enter %d, num %d [%s]",
		rec.pid, rec.enter, rec.scno, sys_name);
    for (int i = 0; i < tcp->u_nargs; i++) {
	const struct sysarg_arg *a = &sa->args[i];
	if (a->type == sysarg_end) {
	    if (record_debug)
		fprintf(stderr, "Syscall arg count mismatch for %ld [%s]\n",
			tcp->scno, sys_name);
	    break;
	}

	if (a->type == sysarg_unknown) {
	    fprintf(stderr, "Unknown arg %d for %ld [%s]\n",
		    i, tcp->scno, sys_name);
	    break;
	}

	if (record_debug)
	    fprintf(stderr, ", %#lx", tcp->u_arg[i]);

	rec.args.args_val[i].argv.argv_len = 0;
	rec.args.args_val[i].argv.argv_val = 0;

	static char bigbuf[32 * 1024 * 1024] = {0};
	static long longbuf;

	char *arg_val;
	int   arg_len;

	switch (a->type) {
	case sysarg_int:
	    arg_val = (char *) &tcp->u_arg[i];
	    arg_len = 8;
	    break;

	case sysarg_strnull:
	    umovestr(tcp, tcp->u_arg[i], sizeof(bigbuf), bigbuf);
	    arg_val = bigbuf;
	    arg_len = strlen(arg_val) + 1;
	    break;

	case sysarg_argv_when_entering:
	case sysarg_argv_when_exiting:
	case sysarg_argv:
	    arg_val = 0;
	    arg_len = 0;

            if ( (a->type == sysarg_argv_when_entering && !entering(tcp))
                 || (a->type == sysarg_argv_when_exiting && entering(tcp)) ) {
                break;
            }

	    for (int idx = 0; ; idx++) {
		long long sptr = 0;
		umoven(tcp, tcp->u_arg[i]+idx*sizeof(sptr), sizeof(sptr), (char *) &sptr);
		if (sptr == 0)
		    break;

		umovestr(tcp, sptr, sizeof(bigbuf), bigbuf);

		rec.args.args_val[i].argv.argv_len++;
		rec.args.args_val[i].argv.argv_val =
		    realloc(rec.args.args_val[i].argv.argv_val,
			    rec.args.args_val[i].argv.argv_len * sizeof(argv_str));
		rec.args.args_val[i].argv.argv_val[rec.args.args_val[i].argv.argv_len-1].s =
		    strdup(bigbuf);
	    }

	    break;

	case sysarg_buf_arglen_when_entering:
	case sysarg_buf_arglen_when_exiting:
	case sysarg_buf_arglen:
	    arg_val = 0;
	    arg_len = 0;

            if ( (a->type == sysarg_buf_arglen_when_entering && !entering(tcp))
                 || (a->type == sysarg_buf_arglen_when_exiting && entering(tcp)) ) {
                break;
            }
            
	    arg_val = bigbuf;
	    arg_len = a->lensz * tcp->u_arg[a->lenarg];
	    assert(arg_len <= sizeof(bigbuf));
	    umoven(tcp, tcp->u_arg[i], arg_len, bigbuf);
	    break;

	case sysarg_buf_arglenp:
	    umoven(tcp, tcp->u_arg[a->lenarg], sizeof(longbuf), (char *) &longbuf);
	    arg_len = a->lensz * longbuf;
	    assert(arg_len <= sizeof(bigbuf));
	    umoven(tcp, tcp->u_arg[i], arg_len, bigbuf);
	    arg_val = bigbuf;
	    break;

	case sysarg_buf_fixlen:
	    arg_val = bigbuf;
	    arg_len = a->lensz;
	    umoven(tcp, tcp->u_arg[i], arg_len, bigbuf);
	    break;

	case sysarg_ignore:
	    /* XXX */
	    arg_val = 0;
	    arg_len = 0;
	    break;

	default:
	    fprintf(stderr, "XXX bad type\n");
	    continue;
	}

	rec.args.args_val[i].data.data_len = arg_len;
	rec.args.args_val[i].data.data_val = malloc(arg_len);
	memcpy(rec.args.args_val[i].data.data_val, arg_val, arg_len);
    }
    if (record_debug)
	fprintf(stderr, ", ret %ld, errno %ld\n", rec.ret, rec.err);

    if (rec.enter && !(rec.scno == 0xc0ffee) &&
	!(rec.scno == SYS_rt_sigprocmask) &&
	!(rec.scno == SYS_rt_sigaction) &&
	!(rec.scno == SYS_rt_sigreturn) &&
	!(rec.scno == SYS_rt_sigtimedwait) &&
	!(rec.scno == SYS_kill) &&
	!(rec.scno == SYS_tkill) &&
	!(rec.scno == SYS_tgkill) &&
	!(rec.scno == SYS_fsync) &&
	!(rec.scno == SYS_fdatasync) &&
	!(rec.scno == SYS_msync) &&
	!(rec.scno == SYS_close) &&
	!(rec.scno == SYS_shutdown) &&
	!(rec.scno == SYS_read) &&
	!(rec.scno == SYS_readv) &&
	!(rec.scno == SYS_pread64) &&
	!(rec.scno == SYS_write  && rec.args.args_val[0].fd_stat && !S_ISREG(rec.args.args_val[0].fd_stat->mode)) &&
	!(rec.scno == SYS_writev && rec.args.args_val[0].fd_stat && !S_ISREG(rec.args.args_val[0].fd_stat->mode)) &&
	!(rec.scno == SYS_pwrite64 && rec.args.args_val[0].fd_stat && !S_ISREG(rec.args.args_val[0].fd_stat->mode)) &&
	!(rec.scno == SYS_open   && !(tcp->u_arg[1] & (O_CREAT | O_TRUNC))) &&
	!(rec.scno == SYS_openat && !(tcp->u_arg[2] & (O_CREAT | O_TRUNC))) &&
	!(rec.scno == SYS_fstat) &&
	!(rec.scno == SYS_newfstatat) &&
	!(rec.scno == SYS_lstat) &&
	!(rec.scno == SYS_statfs) &&
	!(rec.scno == SYS_chmod) &&
	!(rec.scno == SYS_chown) &&
	!(rec.scno == SYS_fchmod) &&
	!(rec.scno == SYS_fchmodat) &&
	!(rec.scno == SYS_fchown) &&
	!(rec.scno == SYS_lchown) &&
	!(rec.scno == SYS_utimensat) &&
	!(rec.scno == SYS_futimesat) &&
	!(rec.scno == SYS_utime) &&
	!(rec.scno == SYS_dup) &&
	!(rec.scno == SYS_dup2) &&
	!(rec.scno == SYS_readlink) &&
	!(rec.scno == SYS_getdents) &&
	!(rec.scno == SYS_arch_prctl) &&
	!(rec.scno == SYS_execve) &&
	!(rec.scno == SYS_clone) &&
	!(rec.scno == SYS_vfork) &&
	!(rec.scno == SYS_fork) &&
	!(rec.scno == SYS_exit) &&
	!(rec.scno == SYS_exit_group) &&
	!(rec.scno == SYS_set_robust_list) &&
	!(rec.scno == SYS_set_tid_address) &&
	!(rec.scno == SYS_flock) &&
	!(rec.scno == SYS_fcntl) &&
	!(rec.scno == SYS_ioctl) &&
	!(rec.scno == SYS_futex) &&
	!(rec.scno == SYS_wait4) &&
	!(rec.scno == SYS_access) &&
	!(rec.scno == SYS_stat) &&
	!(rec.scno == SYS_munmap) &&
	!(rec.scno == SYS_brk) &&
	!(rec.scno == SYS_lseek) &&
	!(rec.scno == SYS_vhangup) &&
	!(rec.scno == SYS_nanosleep) &&

	!(rec.scno == SYS_select) &&
	!(rec.scno == SYS_pselect6) &&
	!(rec.scno == SYS_poll) &&
	!(rec.scno == SYS_epoll_create) &&
	!(rec.scno == SYS_epoll_ctl) &&
	!(rec.scno == SYS_epoll_wait) &&
	!(rec.scno == SYS_epoll_pwait) &&
	!(rec.scno == SYS_socket) &&
	!(rec.scno == SYS_connect) &&
	!(rec.scno == SYS_accept) &&
	!(rec.scno == SYS_listen) &&
	!(rec.scno == SYS_bind) &&
	!(rec.scno == SYS_getpeername) &&
	!(rec.scno == SYS_getsockname) &&
	!(rec.scno == SYS_recvmsg) &&
	!(rec.scno == SYS_sendmsg) &&
	!(rec.scno == SYS_recvfrom) &&
	!(rec.scno == SYS_sendto) &&
	!(rec.scno == SYS_getsockopt) &&
	!(rec.scno == SYS_setsockopt) &&
	!(rec.scno == SYS_pipe) &&
	!(rec.scno == SYS_socketpair) &&

	!(rec.scno == SYS_getuid) &&
	!(rec.scno == SYS_geteuid) &&
	!(rec.scno == SYS_getgid) &&
	!(rec.scno == SYS_getegid) &&
	!(rec.scno == SYS_getpgrp) &&
	!(rec.scno == SYS_getsid) &&
	!(rec.scno == SYS_getcwd) &&
	!(rec.scno == SYS_chdir) &&
	!(rec.scno == SYS_fchdir) &&
	!(rec.scno == SYS_getgroups) &&
	!(rec.scno == SYS_setgroups) &&
	!(rec.scno == SYS_getrusage) &&
	!(rec.scno == SYS_getrlimit) &&
	!(rec.scno == SYS_setrlimit) &&
	!(rec.scno == SYS_madvise) &&
	!(rec.scno == SYS_uname) &&
	!(rec.scno == SYS_umask) &&
	!(rec.scno == SYS_getpid) &&
	!(rec.scno == SYS_getppid) &&
	!(rec.scno == SYS_gettid) &&
	!(rec.scno == SYS_getpriority) &&
	!(rec.scno == SYS_setpriority) &&
	!(rec.scno == SYS_setuid) &&
	!(rec.scno == SYS_setgid) &&
	!(rec.scno == SYS_setpgid) &&
	!(rec.scno == SYS_setresuid) &&
	!(rec.scno == SYS_setresgid) &&
	!(rec.scno == SYS_setreuid) &&
	!(rec.scno == SYS_setregid) &&
	!(rec.scno == SYS_setsid) &&
	!(rec.scno == SYS_chroot) &&

	!(rec.scno == SYS_undo_mask_start) &&
	!(rec.scno == SYS_undo_mask_end) &&
	!(rec.scno == SYS_undo_func_start) &&
	!(rec.scno == SYS_undo_func_end) &&
	!(rec.scno == SYS_undo_depend && tcp->u_arg[3] == 0) &&

	!(rec.scno == SYS_mmap && !(tcp->u_arg[2] & PROT_WRITE)) &&
	!(rec.scno == SYS_mmap && ((tcp->u_arg[3] & MAP_TYPE) == MAP_PRIVATE)) &&
	!(rec.scno == SYS_mprotect && !(tcp->u_arg[2] & PROT_WRITE)) &&
	!(rec.scno == SYS_alarm))
    {
	// record snap_info
	// 1. zero or one file content (with id), e.g., file modified
	// 2. some directory entries, e.g., file created/deleted
	static int snap_count;
	snap_count++;

	char buf[2048];
	char dst_pn[1024] = "/dev/null";
	// record a snapshot of dst_pn

	switch (rec.scno) {
	case SYS_write:
	case SYS_writev:
	case SYS_pwrite64:
	case SYS_undo_depend:
	case SYS_ftruncate:
	    // fd
	    snprintf(dst_pn, sizeof(dst_pn), "/proc/%d/fd/%ld", tcp->pid, tcp->u_arg[0]);
	    break;
	case SYS_mmap:
	    // fd, too
	    snprintf(dst_pn, sizeof(dst_pn), "/proc/%d/fd/%ld", tcp->pid, tcp->u_arg[4]);
	    break;
	case SYS_truncate:
	    // record the old content
	    umovepath(dst_pn, sizeof(dst_pn), tcp, 0, AT_FDCWD);
	    break;
	case SYS_open:
	case SYS_creat:
	    // record the old content
	    // add a new entry when given O_CREAT
	    umovepath(dst_pn, sizeof(dst_pn), tcp, 0, AT_FDCWD);
	    record_snap_dirents(&rec, dst_pn);
	    break;
	case SYS_openat:
	    umovepath(dst_pn, sizeof(dst_pn), tcp, 1, tcp->u_arg[0]);
	    record_snap_dirents(&rec, dst_pn);
	    break;
	case SYS_mkdir:
	    umovepath(buf, sizeof(buf), tcp, 0, AT_FDCWD);
	    record_snap_dirents(&rec, buf);
	    break;
	case SYS_link:
	case SYS_symlink:
	    umovepath(buf, sizeof(buf), tcp, 1, AT_FDCWD);
	    record_snap_dirents(&rec, buf);
	    break;
	case SYS_linkat:
	    umovepath(buf, sizeof(buf), tcp, 3, tcp->u_arg[2]);
	    record_snap_dirents(&rec, buf);
	    break;
	case SYS_symlinkat:
	    umovepath(buf, sizeof(buf), tcp, 2, tcp->u_arg[1]);
	    record_snap_dirents(&rec, buf);
	    break;
	case SYS_unlink:
	case SYS_rmdir:
	    // no content recording
	    // remove an entry
	    // keep a reference
	    umovepath(buf, sizeof(buf), tcp, 0, AT_FDCWD);
	    record_snap_dirents(&rec, buf);
	    linkpath(buf);
	    break;
	case SYS_unlinkat:
	    linkpath(buf);
	case SYS_mkdirat:
	    umovepath(buf, sizeof(buf), tcp, 1, tcp->u_arg[0]);
	    record_snap_dirents(&rec, buf);
	    break;
	case SYS_rename:
	    // no content recording
	    // remove the src entry
	    // add the dst entry
	    // keep a reference to dst
	    umovepath(buf, sizeof(buf), tcp, 0, AT_FDCWD);
	    record_snap_dirents(&rec, buf);
	    umovepath(buf, sizeof(buf), tcp, 1, AT_FDCWD);
	    record_snap_dirents(&rec, buf);
	    linkpath(buf);
	    break;
	case SYS_renameat:
	    umovepath(buf, sizeof(buf), tcp, 1, tcp->u_arg[0]);
	    record_snap_dirents(&rec, buf);
	    umovepath(buf, sizeof(buf), tcp, 3, tcp->u_arg[2]);
	    record_snap_dirents(&rec, buf);
	    linkpath(buf);
	    break;
	default:
	    fprintf(stderr, "snapshot for unknown syscall %s %ld\n", \
	            rec.scname, tcp->scno);
	    break;
	}

	stat_info **ppst = &rec.snap.file_stat;
	record_path_stat_a(ppst, dst_pn, tcp, -1);
	if (tcp->replaymode == 0 && *ppst && S_ISREG((*ppst)->mode)) {
	    /* Try not to snapshot /dev/zero.. */
	    int pid = getpid();
	    rec.snap.id = (((uint64_t)pid) << 32) | snap_count;
	    sprintf(buf, "/bin/cp %s /mnt/undofs/snap/%d.%d", dst_pn, pid, snap_count);
	    int ignore = system(buf);
	    (void) ignore;
	}
    }

    xdr_syscall_record(&xdrs, &rec);
    for (int i = 0; i < rec.args.args_len; i++) {
	if (rec.args.args_val[i].data.data_val)
	    free(rec.args.args_val[i].data.data_val);
	if (rec.args.args_val[i].argv.argv_val) {
	    for (int j = 0; j < rec.args.args_val[i].argv.argv_len; j++)
		free(rec.args.args_val[i].argv.argv_val[j].s);
	    free(rec.args.args_val[i].argv.argv_val);
	}
	if (rec.args.args_val[i].namei) {
	    for (int j = 0; j < rec.args.args_val[i].namei->ni.ni_len; j++)
		if (rec.args.args_val[i].namei->ni.ni_val[j].name)
		    free(rec.args.args_val[i].namei->ni.ni_val[j].name);
	    free(rec.args.args_val[i].namei->ni.ni_val);
	    free(rec.args.args_val[i].namei);
	}
	if (rec.args.args_val[i].fd_stat)
	    free(rec.args.args_val[i].fd_stat);
    }
    free(rec.args.args_val);
    if (rec.ret_fd_stat)
	free(rec.ret_fd_stat);
    if (rec.snap.file_stat)
	free(rec.snap.file_stat);
    if (rec.snap.dirents.dirents_val) {
	for (int i = 0; i < rec.snap.dirents.dirents_len; ++i) {
	    dirent_infos *di = &rec.snap.dirents.dirents_val[i];
	    for (int j = 0; j < di->ni.ni_len; ++j)
		free(di->ni.ni_val[j].name);
	    free(di->ni.ni_val);
	}
	free(rec.snap.dirents.dirents_val);
    }

    if (rec.execi) {
	if (rec.execi->fds.fds_val)
	    free(rec.execi->fds.fds_val);
	free(rec.execi);
    }

    wait_for_command(tcp);
}

static void
wait_for_command(struct tcb *tcp)
{
    if (!tcp->replaymode)
	return;

    for (;;) {
	char code;
	assert(1 == read(0, &code, 1));

	/* Small control language:
	 * c: continue execution
	 * x: exit, change syscall to __NR_exit
	 * s: skip, change syscall to __NR_getpid
	 * r <8 byte rval>: change return value
	 * w <8 byte addr> <1 byte data>: write data to process
	 * W <8 byte argnum> <8 byte offset> <1 byte data>: write rel to arg
	 */

	if (code == 'c') {
	    /* continue */
	    return;
	} else if (code == 's') {
	    /* skip -- only works on syscall entry */
	    struct user_regs_struct r;
	    if (ptrace(PTRACE_GETREGS, tcp->pid, 0, &r) < 0)
		perror("PTRACE_GETREGS");
	    r.orig_rax = __NR_getpid;
	    if (ptrace(PTRACE_SETREGS, tcp->pid, 0, &r) < 0)
		perror("PTRACE_SETREGS");
	} else if (code == 'x') {
	    /* exit -- only works on syscall entry */
	    struct user_regs_struct r;
	    if (ptrace(PTRACE_GETREGS, tcp->pid, 0, &r) < 0)
		perror("PTRACE_GETREGS");
	    r.orig_rax = __NR_exit;
	    if (ptrace(PTRACE_SETREGS, tcp->pid, 0, &r) < 0)
		perror("PTRACE_SETREGS");
	    internal_exit(tcp);
	} else if (code == 'r') {
	    /* return value -- only works on syscall exit */
	    long ret;
	    assert(8 == read(0, &ret, 8));

	    struct user_regs_struct r;
	    if (ptrace(PTRACE_GETREGS, tcp->pid, 0, &r) < 0)
		perror("PTRACE_GETREGS");
	    r.rax = ret;
	    if (ptrace(PTRACE_SETREGS, tcp->pid, 0, &r) < 0)
		perror("PTRACE_SETREGS");
	} else if (code == 'w') {
	    /* write memory */
	    long addr;
	    char b;

	    assert(8 == read(0, &addr, 8));
	    assert(1 == read(0, &b, 1));

	    long v = ptrace(PTRACE_PEEKDATA, tcp->pid, addr, 0);
	    char *vp = (char *) &v;
	    vp[0] = b;
	    ptrace(PTRACE_POKEDATA, tcp->pid, addr, v);
	} else if (code == 'W') {
	    long argbase;
	    long addr;
	    char b;

	    assert(8 == read(0, &argbase, 8));
	    assert(8 == read(0, &addr, 8));
	    assert(1 == read(0, &b, 1));

	    addr += tcp->u_arg[argbase];

	    long v = ptrace(PTRACE_PEEKDATA, tcp->pid, addr, 0);
	    char *vp = (char *) &v;
	    vp[0] = b;
	    ptrace(PTRACE_POKEDATA, tcp->pid, addr, v);
	} else {
	    fprintf(stderr, "unknown command <%c>\n", code);
	}
    }
}

int
trace_syscall(struct tcb *tcp)
{
	int sys_res;
	struct timeval tv;
	int res, scno_good;

	if (tcp->flags & TCB_INSYSCALL) {
		long u_error;

		/* Measure the exit time as early as possible to avoid errors. */
		if (dtime)
			gettimeofday(&tv, NULL);

		/* BTW, why we don't just memorize syscall no. on entry
		 * in tcp->something?
		 */
		scno_good = res = get_scno(tcp);
		if (res == 0)
			return res;
		if (res == 1)
			res = syscall_fixup(tcp);
		if (res == 0)
			return res;
		if (res == 1)
			res = get_error(tcp);
		if (res == 0)
			return res;
		if (res == 1)
			internal_syscall(tcp);

		record_syscall(tcp);
		tcp->flags &= ~TCB_INSYSCALL;
		return res;

		if (res == 1 && tcp->scno >= 0 && tcp->scno < nsyscalls &&
		    !(qual_flags[tcp->scno] & QUAL_TRACE)) {
			tcp->flags &= ~TCB_INSYSCALL;
			return 0;
		}

		if (tcp->flags & TCB_REPRINT) {
			printleader(tcp);
			tprintf("<... ");
			if (scno_good != 1)
				tprintf("????");
			else if (tcp->scno >= nsyscalls || tcp->scno < 0)
				tprintf("syscall_%lu", tcp->scno);
			else
				tprintf("%s", sysent[tcp->scno].sys_name);
			tprintf(" resumed> ");
		}

		if (cflag)
			return count_syscall(tcp, &tv);

		if (res != 1) {
			tprintf(") ");
			tabto(acolumn);
			tprintf("= ? <unavailable>");
			printtrailer();
			tcp->flags &= ~TCB_INSYSCALL;
			return res;
		}

		if (tcp->scno >= nsyscalls || tcp->scno < 0
		    || (qual_flags[tcp->scno] & QUAL_RAW))
			sys_res = printargs(tcp);
		else {
			if (not_failing_only && tcp->u_error)
				return 0;	/* ignore failed syscalls */
			sys_res = (*sysent[tcp->scno].sys_func)(tcp);
		}
		u_error = tcp->u_error;
		tprintf(") ");
		tabto(acolumn);
		if (tcp->scno >= nsyscalls || tcp->scno < 0 ||
		    qual_flags[tcp->scno] & QUAL_RAW) {
			if (u_error)
				tprintf("= -1 (errno %ld)", u_error);
			else
				tprintf("= %#lx", tcp->u_rval);
		}
		else if (!(sys_res & RVAL_NONE) && u_error) {
			switch (u_error) {
#ifdef LINUX
			case ERESTARTSYS:
				tprintf("= ? ERESTARTSYS (To be restarted)");
				break;
			case ERESTARTNOINTR:
				tprintf("= ? ERESTARTNOINTR (To be restarted)");
				break;
			case ERESTARTNOHAND:
				tprintf("= ? ERESTARTNOHAND (To be restarted)");
				break;
			case ERESTART_RESTARTBLOCK:
				tprintf("= ? ERESTART_RESTARTBLOCK (To be restarted)");
				break;
#endif /* LINUX */
			default:
				tprintf("= -1 ");
				if (u_error < 0)
					tprintf("E??? (errno %ld)", u_error);
				else if (u_error < nerrnos)
					tprintf("%s (%s)", errnoent[u_error],
						strerror(u_error));
				else
					tprintf("ERRNO_%ld (%s)", u_error,
						strerror(u_error));
				break;
			}
			if ((sys_res & RVAL_STR) && tcp->auxstr)
				tprintf(" (%s)", tcp->auxstr);
		}
		else {
			if (sys_res & RVAL_NONE)
				tprintf("= ?");
			else {
				switch (sys_res & RVAL_MASK) {
				case RVAL_HEX:
					tprintf("= %#lx", tcp->u_rval);
					break;
				case RVAL_OCTAL:
					tprintf("= %#lo", tcp->u_rval);
					break;
				case RVAL_UDECIMAL:
					tprintf("= %lu", tcp->u_rval);
					break;
				case RVAL_DECIMAL:
					tprintf("= %ld", tcp->u_rval);
					break;
#ifdef HAVE_LONG_LONG
				case RVAL_LHEX:
					tprintf("= %#llx", tcp->u_lrval);
					break;
				case RVAL_LOCTAL:
					tprintf("= %#llo", tcp->u_lrval);
					break;
				case RVAL_LUDECIMAL:
					tprintf("= %llu", tcp->u_lrval);
					break;
				case RVAL_LDECIMAL:
					tprintf("= %lld", tcp->u_lrval);
					break;
#endif
				default:
					fprintf(stderr,
						"invalid rval format\n");
					break;
				}
			}
			if ((sys_res & RVAL_STR) && tcp->auxstr)
				tprintf(" (%s)", tcp->auxstr);
		}
		if (dtime) {
			tv_sub(&tv, &tv, &tcp->etime);
			tprintf(" <%ld.%06ld>",
				(long) tv.tv_sec, (long) tv.tv_usec);
		}
		printtrailer();

		dumpio(tcp);
		if (fflush(tcp->outf) == EOF)
			return -1;
		tcp->flags &= ~TCB_INSYSCALL;
		return 0;
	}

	/* Entering system call */
	scno_good = res = get_scno(tcp);
	if (res == 0)
		return res;
	if (res == 1)
		res = syscall_fixup(tcp);
	if (res == 0)
		return res;
	if (res == 1)
		res = syscall_enter(tcp);
	if (res == 0)
		return res;

	if (res != 1) {
		record_syscall(tcp);
		tcp->flags |= TCB_INSYSCALL;
		return res;

		printleader(tcp);
		tcp->flags &= ~TCB_REPRINT;
		tcp_last = tcp;
		if (scno_good != 1)
			tprintf("????" /* anti-trigraph gap */ "(");
		else if (tcp->scno >= nsyscalls || tcp->scno < 0)
			tprintf("syscall_%lu(", tcp->scno);
		else
			tprintf("%s(", sysent[tcp->scno].sys_name);
		/*
		 * " <unavailable>" will be added later by the code which
		 * detects ptrace errors.
		 */
		tcp->flags |= TCB_INSYSCALL;
		return res;
	}

	switch (known_scno(tcp)) {
#ifdef SYS_socket_subcall
	case SYS_socketcall:
		decode_subcall(tcp, SYS_socket_subcall,
			SYS_socket_nsubcalls, deref_style);
		break;
#endif
#ifdef SYS_ipc_subcall
	case SYS_ipc:
		decode_subcall(tcp, SYS_ipc_subcall,
			SYS_ipc_nsubcalls, shift_style);
		break;
#endif
#ifdef SVR4
#ifdef SYS_pgrpsys_subcall
	case SYS_pgrpsys:
		decode_subcall(tcp, SYS_pgrpsys_subcall,
			SYS_pgrpsys_nsubcalls, shift_style);
		break;
#endif /* SYS_pgrpsys_subcall */
#ifdef SYS_sigcall_subcall
	case SYS_sigcall:
		decode_subcall(tcp, SYS_sigcall_subcall,
			SYS_sigcall_nsubcalls, mask_style);
		break;
#endif /* SYS_sigcall_subcall */
	case SYS_msgsys:
		decode_subcall(tcp, SYS_msgsys_subcall,
			SYS_msgsys_nsubcalls, shift_style);
		break;
	case SYS_shmsys:
		decode_subcall(tcp, SYS_shmsys_subcall,
			SYS_shmsys_nsubcalls, shift_style);
		break;
	case SYS_semsys:
		decode_subcall(tcp, SYS_semsys_subcall,
			SYS_semsys_nsubcalls, shift_style);
		break;
#if 0 /* broken */
	case SYS_utssys:
		decode_subcall(tcp, SYS_utssys_subcall,
			SYS_utssys_nsubcalls, shift_style);
		break;
#endif
	case SYS_sysfs:
		decode_subcall(tcp, SYS_sysfs_subcall,
			SYS_sysfs_nsubcalls, shift_style);
		break;
	case SYS_spcall:
		decode_subcall(tcp, SYS_spcall_subcall,
			SYS_spcall_nsubcalls, shift_style);
		break;
#ifdef SYS_context_subcall
	case SYS_context:
		decode_subcall(tcp, SYS_context_subcall,
			SYS_context_nsubcalls, shift_style);
		break;
#endif /* SYS_context_subcall */
#ifdef SYS_door_subcall
	case SYS_door:
		decode_subcall(tcp, SYS_door_subcall,
			SYS_door_nsubcalls, door_style);
		break;
#endif /* SYS_door_subcall */
#ifdef SYS_kaio_subcall
	case SYS_kaio:
		decode_subcall(tcp, SYS_kaio_subcall,
			SYS_kaio_nsubcalls, shift_style);
		break;
#endif
#endif /* SVR4 */
#ifdef FREEBSD
	case SYS_msgsys:
	case SYS_shmsys:
	case SYS_semsys:
		decode_subcall(tcp, 0, 0, table_style);
		break;
#endif
#ifdef SUNOS4
	case SYS_semsys:
		decode_subcall(tcp, SYS_semsys_subcall,
			SYS_semsys_nsubcalls, shift_style);
		break;
	case SYS_msgsys:
		decode_subcall(tcp, SYS_msgsys_subcall,
			SYS_msgsys_nsubcalls, shift_style);
		break;
	case SYS_shmsys:
		decode_subcall(tcp, SYS_shmsys_subcall,
			SYS_shmsys_nsubcalls, shift_style);
		break;
#endif
	}

	internal_syscall(tcp);

	record_syscall(tcp);
	tcp->flags |= TCB_INSYSCALL;
	return res;

	if (tcp->scno >=0 && tcp->scno < nsyscalls && !(qual_flags[tcp->scno] & QUAL_TRACE)) {
		tcp->flags |= TCB_INSYSCALL;
		return 0;
	}

	if (cflag) {
		gettimeofday(&tcp->etime, NULL);
		tcp->flags |= TCB_INSYSCALL;
		return 0;
	}

	printleader(tcp);
	tcp->flags &= ~TCB_REPRINT;
	tcp_last = tcp;
	if (tcp->scno >= nsyscalls || tcp->scno < 0)
		tprintf("syscall_%lu(", tcp->scno);
	else
		tprintf("%s(", sysent[tcp->scno].sys_name);
	if (tcp->scno >= nsyscalls || tcp->scno < 0 ||
	    ((qual_flags[tcp->scno] & QUAL_RAW) && tcp->scno != SYS_exit))
		sys_res = printargs(tcp);
	else
		sys_res = (*sysent[tcp->scno].sys_func)(tcp);
	if (fflush(tcp->outf) == EOF)
		return -1;
	tcp->flags |= TCB_INSYSCALL;
	/* Measure the entrance time as late as possible to avoid errors. */
	if (dtime)
		gettimeofday(&tcp->etime, NULL);
	return sys_res;
}

int
printargs(tcp)
struct tcb *tcp;
{
	if (entering(tcp)) {
		int i;

		for (i = 0; i < tcp->u_nargs; i++)
			tprintf("%s%#lx", i ? ", " : "", tcp->u_arg[i]);
	}
	return 0;
}

long
getrval2(tcp)
struct tcb *tcp;
{
	long val = -1;

#ifdef LINUX
#if defined (SPARC) || defined (SPARC64)
	struct pt_regs regs;
	if (ptrace(PTRACE_GETREGS,tcp->pid,(char *)&regs,0) < 0)
		return -1;
	val = regs.u_regs[U_REG_O1];
#elif defined(SH)
	if (upeek(tcp, 4*(REG_REG0+1), &val) < 0)
		return -1;
#elif defined(IA64)
	if (upeek(tcp, PT_R9, &val) < 0)
		return -1;
#endif
#endif /* LINUX */

#ifdef SUNOS4
	if (upeek(tcp, uoff(u_rval2), &val) < 0)
		return -1;
#endif /* SUNOS4 */

#ifdef SVR4
#ifdef SPARC
	val = tcp->status.PR_REG[R_O1];
#endif /* SPARC */
#ifdef I386
	val = tcp->status.PR_REG[EDX];
#endif /* I386 */
#ifdef X86_64
	val = tcp->status.PR_REG[RDX];
#endif /* X86_64 */
#ifdef MIPS
	val = tcp->status.PR_REG[CTX_V1];
#endif /* MIPS */
#endif /* SVR4 */

#ifdef FREEBSD
	struct reg regs;
	pread(tcp->pfd_reg, &regs, sizeof(regs), 0);
	val = regs.r_edx;
#endif
	return val;
}

#ifdef SUNOS4
/*
 * Apparently, indirect system calls have already be converted by ptrace(2),
 * so if you see "indir" this program has gone astray.
 */
int
sys_indir(tcp)
struct tcb *tcp;
{
	int i, scno, nargs;

	if (entering(tcp)) {
		if ((scno = tcp->u_arg[0]) > nsyscalls) {
			fprintf(stderr, "Bogus syscall: %u\n", scno);
			return 0;
		}
		nargs = sysent[scno].nargs;
		tprintf("%s", sysent[scno].sys_name);
		for (i = 0; i < nargs; i++)
			tprintf(", %#lx", tcp->u_arg[i+1]);
	}
	return 0;
}
#endif /* SUNOS4 */

int
is_restart_error(struct tcb *tcp)
{
#ifdef LINUX
	if (!syserror(tcp))
		return 0;
	switch (tcp->u_error) {
		case ERESTARTSYS:
		case ERESTARTNOINTR:
		case ERESTARTNOHAND:
		case ERESTART_RESTARTBLOCK:
			return 1;
		default:
			break;
	}
#endif /* LINUX */
	return 0;
}
