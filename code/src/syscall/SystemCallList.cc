// Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

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

void SystemCallList::init(istream &in, FDManager *fdManager)
{
    // '\n' is used as a delimeter between syscall's
    string syscallString;
    SystemCall syscall;
    bool usage = false;
    while (!getline(in, syscallString).eof())
    {
        SystemCall syscall(syscallString, usage, fdManager);
        syscallVector.push_back(syscall);
        usage = !usage;
    }
}

String SystemCallList::toString()
{
    ostringstream ss;
    ss << "SystemCallList:" << endl;
    Vector<SystemCall>::iterator it;
    for (it = syscallVector.begin(); it != syscallVector.end(); ++it)
    {
        ss << it->toString() << endl;
    }
    return ss.str();
}

