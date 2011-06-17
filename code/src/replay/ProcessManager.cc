//Author: Hao Wu <haowu@cs.utexas.edu>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <vector>

#include <common.h>
#include "ProcessManager.h"
using namespace std;

ProcessManager::ProcessManager(Vector<String> &command, SystemCallList &list)
    : commandList(command), syscallList(list)
{
}

int ProcessManager::replay()
{
    // XXX: Somehow we need to specify some arguments here in the future.
    return startProcess();
}

int ProcessManager::startProcess()
{
    // If the command line is empty, we cannot do anything
    if (commandList.empty())
    {
        LOG1("Command is empty, refrain from executing nothing.");
        return -1;
    }
    pid_t pid = fork();

    // If the fork fails
    if (pid == -1)
    {
        LOG("fork fails when trying to run %s", toString().c_str());
        return -1;
    }

    // If it is the child process
    if (pid == 0)
    {
        return executeProcess();
    }
    // If it is the parent process
    else
    {
        return traceProcess(pid);
    }
}

int ProcessManager::executeProcess()
{
    // Assert the command is not empty here.
    ASSERT(commandList.size() != 0);
    char **args = new char *[commandList.size()+1];
    LOG1("This is the child process!");
    long pret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    for (int i = 0; i < commandList.size(); i++)
    {
        // XXX: This is not very clean. But we have to copy `argv' into a new char array since
        // `char *' is required in execvp, but the type of `argv' is `const char *'.
        const char *argv = commandList[i].c_str();
        args[i] = new char[strlen(argv)];
        strcpy(args[i], argv);
    }
    args[commandList.size()] = NULL;

    // Execute the command
    int ret = execvp(commandList[0].c_str(), args);

    for (int i = 0; i < commandList.size(); i++)
    {
        delete args[i];
    }
    return ret;
}

int ProcessManager::traceProcess(pid_t pid)
{
    LOG1("This is the parent process!");
    int status;
    waitpid(pid, &status, 0);
    long pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    if (pret < 0)
    {
        //do nothing
    }
    LOG1("This is the parent process!");
    return 0;
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
        return -1;
    }

    Vector<String> commands;
    for (int i = 1; i < argc; i++)
    {
        commands.push_back(string(argv[i]));
    }
    SystemCallList list;

    ProcessManager manager(commands, list);
    //cout << manager.toString();
    manager.replay();
    return 0;
}

