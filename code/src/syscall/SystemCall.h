// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALL_H__
#define __SYSCALL_SYSCALL_H__

// The class is used to represent the **record** of a system call
// @author haowu
class SystemCall
{
    public:
        SystemCall() : valid(false) {}
        // Construct the system call from registers
        SystemCall(user_regs_struct regs);

        // Whether the system call is valid.
        // A valid system call is a system with its code and args provided.
        // @author haowu
        bool isValid() { return valid;}
    private:
        bool valid;
};

#endif //__SYSCALL_SYSCALL_H__
