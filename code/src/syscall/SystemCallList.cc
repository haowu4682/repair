// Author: Hao Wu <haowu@cs.utexas.edu>

#include <syscall/SystemCall.h>
#include <syscall/SystemCallList.h>
using namespace std;

SystemCall SystemCallList::search(SystemCall &syscall)
{
    // TODO: devide by pid
    SystemCall emptySyscall;
    Vector<SystemCall>::iterator it;
    for (it = syscallVector.begin(); it < syscallVector.end(); ++it)
    {
        if (*it == syscall)
        {
            return *it;
        }
    }
    return emptySyscall;
}

void SystemCallList::init(istream &in)
{
    // '\n' is used as a delimeter between syscall's
    string syscallString;
    SystemCall syscall;
    bool usage = false;
    while (getline(in, syscallString) == 0)
    {
        SystemCall syscall(syscallString, usage);
        syscallVector.push_back(syscall);
        usage = !usage;
    }
}

