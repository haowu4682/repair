// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_PROCESSMANAGER_H__
#define __REPLAY_PROCESSMANAGER_H__

#include <string>
#include <vector>

#include <common.h>
#include <syscall/SystemCallList.h>

// This class is used to manager the running or a process of class Process
// @author haowu
class ProcessManager
{
    public:
        // The constructor
        // @author haowu
        ProcessManager(Vector<String> &command, SystemCallList &list);

        // Start replaying the process
        // @author haowu
        int replay();

        // Output all the infomation as a string.
        // The function is used for debugging and logging.
        // @author haowu
        String toString();
    private:
        // Start a process with Ptrace
        // @author haowu
        int StartProcess();
        // The command line
        Vector<String> &commandList;
        // The system call list used when replaying
        SystemCallList &syscallList;
};

#endif //__REPLAY_PROCESSMANAGER_H__

