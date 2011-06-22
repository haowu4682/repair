// Author: Hao Wu <haowu@cs.utexas.edu>

#include <syscall/SystemCall.h>
#include <syscall/SystemCallList.h>
using namespace std;

SystemCall SystemCallList::search(SystemCall &syscall)
{
    // TODO: Fill in the function
    SystemCall matchedSyscall;
    return matchedSyscall;
}

void SystemCallList::init(istream in)
{
    // '\n' is used as a delimeter between syscall's
    string syscallString;
    SystemCall syscall;
    while (getline(in, syscallString) == 0)
    {
        SystemCall syscall(syscallString);
        syscallVector.push_back(syscall);
    }
}
