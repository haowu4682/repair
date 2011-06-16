//Author: Hao Wu <haowu@cs.utexas.edu>

#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <common.h>
#include "ProcessManager.h"
using namespace std;

ProcessManager::ProcessManager(Vector<String> &command, SystemCallList &list)
    : commandList(command), syscallList(list)
{
}

String ProcessManager::toString()
{
    // XXX: We only output the command line currently.
    stringstream ss;
    for (Vector<String>::iterator it = commandList.begin(); it != commandList.end(); it++)
    {
        ss << (*it) << ", ";
    }
    ss << endl;
    return ss.str();
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
    cout << manager.toString();
    return 0;
}

