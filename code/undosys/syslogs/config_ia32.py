#! /usr/bin/python

def each_syscall() :
    beg = False
    for l in open( "/home/taesoo/Working/kernel/linux-2.6.31/arch/x86/ia32/ia32entry.S" ) :
        if l.startswith( "ia32_syscall_end" ) :
            raise StopIteration()
            
        if beg :
            yield l.rsplit()[1]

        if l.startswith( "ia32_sys_call_table" ) :
            beg = True
            continue

blacklists = [
    "quiet_ni_syscall",
    "oldumount",
    "nice",
    "signal",
    "umount",
    "sgetmask",
    "ssetmask",
    "ioperm",
    "uname",
    "modify_ldt",
    "llseek",
    "sendfile64",
    "set_thread_area",
    "get_thread_area",
    "getcpu",
    "perf_counter_open"]

whitelists = ["stub32_execve"]

syscalls = []
for (n,syscall) in enumerate(each_syscall()) :
    if syscall.startswith( "sys_" ) :
        syscall = syscall.replace( "sys_", "" )

    entry = [True, n, syscall]
    if syscall in blacklists or \
           syscall.startswith( "compat" ) or \
           syscall.find( "32" ) >= 0 or \
           syscall.find( "16" ) >= 0 :
        entry[0] = False
        
    syscalls.append( entry )

print """\
void hook_sys_table32(void)
{
    unsigned long flags;
    local_irq_save( flags );
"""

for (enabled,n,syscall) in syscalls :
    # record original syscall table entry
    print "    ",
    if not enabled :
        print "//",
        
    print "orig_sys_call_table32[%d] = sys_call_table32[%d];" % (n, n)

    # overwrite
    print "    ",
    if not enabled :
        print "//",

    print "sys_call_table32[%d] = (unsigned long) hooked_%s;" % (n, syscall)

    
print """\

    local_irq_restore( flags );
}
"""

print """\
void unhook_sys_table32(void)
{
    unsigned long flags;
    local_irq_save( flags );
"""

for (enabled,n,syscall) in syscalls :
    print "    ",
    if not enabled :
        print "//",

    print "sys_call_table32[%d] = orig_sys_call_table32[%d];" % (n,n)

print """\

    local_irq_restore( flags );
}
"""

fd = open( "ia32_syscall_def.h", "w" )

for (enabled,n,syscall) in syscalls :
    if syscall in whitelists :
        if syscall.startswith( "stub32_" ) :
            syscall = syscall.replace( "stub32_", "" )
            enabled = True

    if enabled :
        fd.write( "#define %-30s (%d)\n" % ("__NR32_%s" % syscall,n) )
    

fd.close()

