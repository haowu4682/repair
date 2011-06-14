#!/usr/bin/perl
#
# Usage: extract-linux-sysargs.sh | gen-sysargs.pl > sysarg.c
# then manually fixup sysarg.c

my %typemap = (
    'int',		('sysarg_int'),
    'unsigned int',	('sysarg_int'),
    'long',		('sysarg_int'),
    'unsigned long',	('sysarg_int'),
    'u_long',		('sysarg_int'),
    'unsigned',		('sysarg_int'),
    'u32',		('sysarg_int'),
    'u64',		('sysarg_int'),
    '__s32',		('sysarg_int'),

    'pid_t',		('sysarg_int'),
    'uid_t',		('sysarg_int'),
    'gid_t',		('sysarg_int'),
    'mode_t',		('sysarg_int'),
    'size_t',		('sysarg_int'),
    'off_t',		('sysarg_int'),
    'loff_t',		('sysarg_int'),
    'old_uid_t',	('sysarg_int'),
    'old_gid_t',	('sysarg_int'),
    'key_t',		('sysarg_int'),
    'mqd_t',		('sysarg_int'),
    'timer_t',		('sysarg_int'),
    'qid_t',		('sysarg_int'),
    'key_serial_t',	('sysarg_int'),
    'const clockid_t',	('sysarg_int'),
    'aio_context_t',	('sysarg_int'),

);

sub ctype_guess {
    my ($type, $name) = @_;
    if ((($type =~ /^char __user \*$/) || ($type =~ /^const char __user \*$/)) &&
	(($name =~ /name/) || ($name =~ /path/)))
    {
	return ('sysarg_strnull');
    }

    if (($type =~ /^int __user \*$/) && ($name =~ /len$/)) {
	return ('sysarg_buf_fixlen', 'sizeof(int)');
    }

    if (($type =~ /^uid_t __user \*$/) && ($name =~ /uid$/)) {
	return ('sysarg_buf_fixlen', 'sizeof(uid_t)');
    }

    if (($type =~ /^gid_t __user \*$/) && ($name =~ /gid$/)) {
	return ('sysarg_buf_fixlen', 'sizeof(gid_t)');
    }

    if (($type =~ /^old_uid_t __user \*$/) && ($name =~ /uid$/)) {
	return ('sysarg_buf_fixlen', 'sizeof(old_uid_t)');
    }

    if (($type =~ /^old_gid_t __user \*$/) && ($name =~ /gid$/)) {
	return ('sysarg_buf_fixlen', 'sizeof(old_gid_t)');
    }

    if ($name eq 'statbuf') {
	$type =~ s/ __user \*//;
	return ('sysarg_buf_fixlen', "sizeof($type)");
    }

    my @nonarray_ptrs = ( 'struct timespec', 'struct itimerspec', 'struct itimerval',
			  'struct timeval' );
    foreach $ptype (@nonarray_ptrs) {
	if ($type =~ /(const )?$ptype __user \*$/ && $name ne 'utimes') {
	    return ('sysarg_buf_fixlen', "sizeof($ptype)");
	}
    }

    return undef;
}

print <<EOM;
#include "sysarg.h"
#include "sysarg-types.h"

struct sysarg_call sysarg_calls[] = {
EOM

while (<>) {
    chomp;
    if (/^SYSCALL_DEFINE\(/) {
	s/\)\(/,/;
	s/\s*([\w_]+,)/,$1/g;
	s/\s*([\w_]+\))/,$1/g;
	s/\(,/\(/;
    }

    s/^SYSCALL_DEFINE.?\(//;
    s/\)$//;

    my @def = split(/,\s*/);
    print '    { "' . (shift @def) . '", {' . "\n";

    while (1) {
	last if $#def < 0;

	$type = shift @def;
	$name = shift @def;

	@ctype = $typemap{$type};
	@ctype = ctype_guess($type, $name) unless defined $ctype[0];
	@ctype = ('sysarg_unknown') unless defined $ctype[0];

	print "      { $ctype[0], $ctype[1] }, /* $type $name */\n";
    }

    print "      { sysarg_end } } },\n";
}

print <<EOM;
    { 0 },
};
EOM
