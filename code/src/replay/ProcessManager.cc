//Author: Hao Wu <haowu@cs.utexas.edu>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <vector>

#include <common/common.h>
#include <replay/ProcessManager.h>
#include <syscall/SystemCall.h>
using namespace std;

ProcessManager::ProcessManager(Vector<String> *command, SystemCallList *list)
    : commandList(command), syscallList(list)
{
}

ProcessManager::ProcessManager(SystemCallList *list)
    : syscallList(list), commandList(NULL)
{
}

int ProcessManager::replay()
{
    // XXX: Somehow we need to specify some arguments here in the future.
    return startProcess();
}

int ProcessManager::trace(pid_t pid)
{
    long pret;
    pret = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    if (pret < 0)
    {
        LOG("Cannot attach to process with pid: %d", pid);
        return -1;
    }
    traceProcess(pid);
    // XXX: Maybe this is not necessary, or should be removed
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
}

int ProcessManager::startProcess()
{
    // If the command line is empty, we cannot do anything
    if (commandList->empty())
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
    if (commandList == NULL)
    {
        LOG1("command list is empty!");
        return -1;
    }
    // Assert the command is not empty here.
    ASSERT(commandList->size() != 0);
    LOG1("This is the child process!");

    // Arrange the arguments
    char **args = new char *[commandList->size()+1];
    for (int i = 0; i < commandList->size(); i++)
    {
        // XXX: This is not very clean. But we have to copy `argv' into a new char array since
        // `char *' is required in execvp, but the type of `argv' is `const char *'.
        const char *argv = (*commandList)[i].c_str();
        args[i] = new char[strlen(argv)];
        strcpy(args[i], argv);
    }
    args[commandList->size()] = NULL;

    // Let the process to be traced
    long pret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    if (pret < 0)
    {
        LOG1("The child process cannot be traced!");
        return pret;
    }

    // Execute the command
    int ret;
    ret = execvp((*commandList)[0].c_str(), args);

    // Clean up
    for (int i = 0; i < commandList->size(); i++)
    {
        delete args[i];
    }
    return ret;
}

// Deal with ``fork'' or ``vfork'' in tracing a process.
// @ret 0 when it's the manager of the original process.
//      1 when it's the manager of the new process.
//      <0 when there is an error.
int dealWithFork(SystemCall &syscall, SystemCallList *list)
{
    // TODO: deal with fork
    // XXX: Hard code features for x86_64 here.
    pid_t newManagerPid = fork();
    if (newManagerPid == 0)
    {
        pid_t processPid = (pid_t) syscall.getReturn();
        // Child process, manage the new process;
        ProcessManager manager(list);
        manager.trace(processPid);
        return 1;
    }
    return 0;
}

int ProcessManager::traceProcess(pid_t pid)
{
    LOG1("This is the parent process!");
    int status;
    int ret;
    long pret;
    struct user_regs_struct regs;

    waitpid(pid, &status, 0);
    // TODO: Use more correct condition here
    // Current the termination condition is: the child has exited from executing
    while (!WIFEXITED(status))
    {
        pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        waitpid(pid, &status, 0);
        // The child process is at the point **before** a syscall.
        // TODO: Deal with the syscall here.
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        SystemCall syscall(regs);
        SystemCall syscallMatch = syscallList->search(syscall);
        // If no match has been found, we have to go on executing the system call and simply do
        // nothing else here. However, if a match has been found we must change the return value
        // accoridngly when the syscall has returned.
        bool matchFound = syscallMatch.isValid();

        pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        waitpid(pid, &status, 0);
        // The child process is at the point **after** a syscall.
        // Deal with the syscall here. If the match has been found previously, we shall
        // replace the return value with the recorded value in the systemcall list.
        // Most syscall will have its return value in the register %rax, But there are some
        // which does not follow the rule and we will need to deal with them seperately.
        if (matchFound)
        {
            writeMatchedSyscall(syscallMatch, pid);
        }
        LOG("syscall nr: %lu\n", regs.orig_rax);

        // If the system call is fork/vfork, we must create a new process manager for it.
        if (syscall.isFork())
        {
            ret = dealWithFork(syscall, syscallList);
            if (ret != 0)
            {
                break;
            }
        }
    }
    LOG1("This is the parent process!");
    return 0;
}

int ProcessManager::writeMatchedSyscall(SystemCall syscall, pid_t pid)
{
    // Simply rewrite rax currently
    long pret;
    int ret;
    struct user_regs_struct regs;

    ptrace(PTRACE_GETREGS, pid, 0, &regs);
    ret = syscall.overwrite(regs);
    if (ret < 0)
    {
        LOG1("Cannot put the return value into registers!");
        return ret;
    }
    pret = ptrace(PTRACE_SETREGS, pid, 0, &regs);
    if (pret < 0)
    {
        LOG1("Cannot overwrite the return value!");
        return (int) pret;
    }
    return 0;
}

String ProcessManager::toString()
{
    // XXX: We only output the command line currently.
    stringstream ss;
    for (Vector<String>::iterator it = commandList->begin(); it != commandList->end(); it++)
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

    ProcessManager manager(&commands, &list);
    //cout << manager.toString();
    manager.replay();
    return 0;
}

