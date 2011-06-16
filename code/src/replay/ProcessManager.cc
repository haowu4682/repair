//Author: Hao Wu <haowu@cs.utexas.edu>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <common.h>
#include "ProcessManager.h"
using namespace std;

ProcessManager(Vector<String> &command, SystemCallList &list)
{
    this->command = command;
    this->syscallList = list;
}

// The main function is used for development and debugging only.
// It will be removed in the released version
// @author haowu
int main(int argc, char **argv)
{
    // Init the ProcessManager
    if (argc < 2)
    {
        LOG1("Usage: ProcessManager command [args]");
    }

    Vector<String> commands;
    for (int i = 1; i < argc; i++)
    {
        commands.push_back(string(argv[i]));
    }
    SystemCallList list;

    ProcessManager manager(commands, list);
    return 0;
}

