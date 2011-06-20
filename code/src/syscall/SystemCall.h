// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALL_H__
#define __SYSCALL_SYSCALL_H__

#include <sys/user.h>

// The class is used to represent the **record** of a system call
// @author haowu
class SystemCall
{
    public:
        SystemCall() : valid(false) {}
        // Construct the system call from registers
        SystemCall(const user_regs_struct &regs);

        // Whether the system call is valid.
        // A valid system call is a system with its code and args provided.
        // @author haowu
        bool isValid() { return valid;}

        // Given the current registers, overwrite the return value in the registers
        // with current return values of the system call.
        // @author haowu
        int overwrite(user_regs_struct &regs);
    private:
        bool valid;
};

#endif //__SYSCALL_SYSCALL_H__
