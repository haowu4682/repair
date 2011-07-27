//Author: Hao Wu <haowu@cs.utexas.edu> 
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <vector>

#include <common/common.h>
#include <common/util.h>
#include <replay/ProcessManager.h>
#include <syscall/SystemCall.h>
using namespace std;

// Function for pthread
void *replayProcess(void *manager)
{
    ProcessManager *procManager = (ProcessManager *)manager;
    procManager->replay();
}

int ProcessManager::replay()
{
    return startProcess();
}

// Function for pthread
void *traceProcess(void *process)
{
    ManagedProcess *proc = (ManagedProcess *)process;
    proc->manager->trace(proc->pid);
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
    if (process.getCommand()->argv.empty())
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
    Vector<String> *commandList = &process.getCommand()->argv;
    // Assert the command is not empty here.
    ASSERT(commandList->size() != 0);

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

    // Execute pre-actions
    Vector<Action *> *preActions = process.getPreActions();
    for (Vector<Action *>::iterator it = preActions->begin(); it != preActions->end(); ++it)
    {
        (*it)->exec();
    }

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
int ProcessManager::dealWithFork(SystemCall &syscall, pid_t oldPid)
{
    // XXX: Hard code features for x86_64 here.
    // update pid manager
    pid_t newPid = (pid_t) syscall.getReturn();
    PidManager *pidManager = process.getPidManager();
    SystemCallList *syscallList = process.getSyscallList();
    if (pidManager != NULL)
    {
        pidManager->add(oldPid, newPid);
    }
    // fork a new manager
    pid_t newManagerPid = fork();
    if (newManagerPid == 0)
    {
        // Child process, manage the new process;
        // TODO: trace the process
        /*
        ProcessManager manager(syscallList, pidManager);
        manager.trace(newPid);
        */
        return 1;
    }
    // We don't need to memorize the pid of the new manager here.
    return 0;
}

bool isConflict()
{
    //TODO: determine definition of conflict here.
    return false;
}

void dealWithConflict()
{
    if (isConflict())
    {
    }
}

int ProcessManager::traceProcess(pid_t pid)
{
    //LOG1("This is the parent process!");
    int status;
    int ret;
    long pret;
    struct user_regs_struct regs;
    PidManager *pidManager = process.getPidManager();
    SystemCallList *syscallList = process.getSyscallList();
    FDManager *fdManager = process.getFDManager();

    waitpid(pid, &status, 0);
    pid_t oldPid = process.getCommand()->pid;
    // TODO: generation number
    if (pidManager != NULL && oldPid != -1)
    {
        pidManager->add(oldPid, pid);
    }
    // Current the termination condition is: the child has exited from executing
    while (!WIFEXITED(status))
    {
        pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        waitpid(pid, &status, 0);

        // The child process is at the point **before** a syscall.
        // TODO: Deal with the syscall here.
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        SystemCall syscall(regs, pid, false, fdManager);
        SystemCall syscallMatch = syscallList->search(syscall);

        // If no match has been found, we have to go on executing the system call and simply do
        // nothing else here. However, if a match has been found we must change the return value
        // accoridngly when the syscall has returned.
        bool matchFound = syscallMatch.isValid();
        //LOG("syscall nr: %lu, match found %d", regs.orig_rax, matchFound);

        pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        waitpid(pid, &status, 0);

        // The child process is at the point **after** a syscall.
        // Deal with the syscall here. If the match has been found previously, we shall
        // replace the return value with the recorded value in the systemcall list.
        // Most syscall will have its return value in the register %rax, But there are some
        // which does not follow the rule and we will need to deal with them seperately.
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        SystemCall syscallReturn(regs, pid, true, fdManager);

        // TODO: Deal with conflict
        dealWithConflict();

        // If the system call is fork/vfork, we must create a new process manager for it.
        if (syscall.isFork())
        {
            ret = dealWithFork(syscall, pid);
            if (ret != 0)
            {
                break;
            }
        }
    }
    //LOG1(pidManager->toString().c_str());
    return 0;
}

int ProcessManager::writeMatchedSyscall(SystemCall &syscall, pid_t pid)
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
    // We only output the command line currently.
    stringstream ss;
    // TODO: implement
    /*
    for (Vector<String>::iterator it = commandList->begin(); it != commandList->end(); it++)
    {
        ss << (*it) << ", ";
    }
    ss << endl;
    */
    return ss.str();
}

// The main function is used for development and debugging only.
// It will be removed in the released version
// @author haowu
/*
int old_main(int argc, char **argv)
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
    commands = convertCommand(argc-1, argv+1);
    ifstream fin("/home/haowu/repair_data/dumb.txt");
    PidManager pidManager;
    SystemManager sysManager;
    SystemCallList list(&pidManager, &sysManager);
    ProcessManager manager(&commands, &list, &pidManager);
    sysManager.setSyscallList(&list);
    LOG1(sysManager.toString().c_str());
    sysManager.execAll();

    list.init(fin, manager.getFDManager());
    return 0;
}
*/
