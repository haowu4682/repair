// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALL_H__
#define __SYSCALL_SYSCALL_H__

#include <sys/user.h>

#include <common/common.h>

// The class is used to represent the **record** of a system call
// @author haowu
class SystemCall
{
    public:
        SystemCall() : valid(false) {}
        // Construct the system call from registers
        SystemCall(const user_regs_struct &regs);
        SystemCall(String record) { this->init(record); }

        // Whether the system call is valid.
        // A valid system call is a system with its code and args provided.
        // @author haowu
        bool isValid() { return valid;}

        // Given the current registers, overwrite the return value in the registers
        // with current return values of the system call.
        // @author haowu
        // @param regs The current registers, will be modified.
        int overwrite(user_regs_struct &regs);

        // Init a system call from a record
        // @param record The record
        int init(String record);
    private:
        // If the system call is valid
        bool valid;
        // The system call code
        long code;
        // The system call args
        // `6' is hard-coded here
        long args[6];
        // The return value of the system call
        long ret;
};

#endif //__SYSCALL_SYSCALL_H__
