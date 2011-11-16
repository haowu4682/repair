//Author: Hao Wu <haowu@cs.utexas.edu>
#include <cstring>
#include <ctime>
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
#include <syscall/SystemCallArg.h>
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

#if 0
    // Execute pre-actions
    // XXX: Temporarily canceled for debugging, if bash command could be
    // implemented as user input, we finally do not need to deal with
    // pre-actions.
    LOG("Before executing pre-actions %s", process.getCommand()->toString().c_str());
    Vector<Action *> *preActions = process.getPreActions();
    for (Vector<Action *>::iterator it = preActions->begin(); it != preActions->end(); ++it)
    {
        (*it)->exec();
    }
    LOG("After executing pre-actions %s %ld", process.getCommand()->toString().c_str(), preActions->size());
#endif

    // Let the process to be traced
    long pret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    if (pret < 0)
    {
        LOG1("The child process cannot be traced!");
        return pret;
    }

    // Execute the command
    int ret;
    LOG("Before executing %s", process.getCommand()->toString().c_str());
    ret = execvp((*commandList)[0].c_str(), args);

    // Clean up
    for (int i = 0; i < commandList->size(); i++)
    {
        delete args[i];
    }
    LOG("Finished executing %s", process.getCommand()->toString().c_str());
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
        // TODO: implement
    }
}

int ProcessManager::traceProcess(pid_t pid)
{
    int status;
    int ret;
    long pret;
    long inputSeqNum = 0;
    struct user_regs_struct regs;
    PidManager *pidManager = process.getPidManager();
    SystemCallList *syscallList = process.getSyscallList();
    FDManager *fdManager = process.getFDManager();

    waitpid(pid, &status, 0);
    pid_t oldPid = process.getCommand()->pid;

    // XXX: generation numbers might cause problems here.
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
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        SystemCall syscallMatch;
        SystemCall syscall(regs, pid, SYSARG_IFENTER, fdManager);

        // If the system call is user input or output, we need to act quite differently.
        if (syscall.isUserSelect(true))
        {
            LOG("User select found: %s", syscall.toString().c_str());
            pret = syscallList->searchMatchInput(syscallMatch, syscall, oldPid, inputSeqNum);
            LOG("match return value = %ld", pret);
            bool matchFound = (pret >= 0);

            // If a match has been found, we'll change the syscall result
            // accordingly and cancel the syscall execution. Otherwise the
            // syscall is executed as usual.
            if (matchFound)
            {
                LOG("Match Found! Match is: %s", syscallMatch.toString().c_str());
                inputSeqNum = pret;
                // We do not skip executing the system call! We hack it to
                // execute in no-timeout way.
                timeval zeroTime = {0, 0};
                writeToProcess(&zeroTime, SystemCall::getArgFromReg(regs, 4),
                        sizeof(timeval), pid);
                pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                waitpid(pid, &status, 0);

                // Achieve the result of the execution
                ptrace(PTRACE_GETREGS, pid, 0, &regs);
                SystemCall syscallReturn(regs, pid, SYSARG_IFEXIT, fdManager);

                // Merge the result with recorded result
                //mergeSystemCall(syscallReturn, syscallMatch);
                syscallReturn.merge(syscallMatch);

                // Write matched system call
                // Get the user input from syscallMatch
                // Use ptrace to put the user input back
                writeMatchedSyscall(syscallReturn, pid);
            }
            else
            {
                LOG("No Match Found!");
                pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                waitpid(pid, &status, 0);
            }
        }
        else if (syscall.isRegularUserInput())
        {
            LOG("User input found: %s", syscall.toString().c_str());
            pret = syscallList->searchMatchInput(syscallMatch, syscall, oldPid, inputSeqNum);
            bool matchFound = (pret >= 0);

            // If a match has been found, we'll change the syscall result
            // accordingly and cancel the syscall execution. Otherwise the
            // syscall is executed as usual.
            if (matchFound)
            {
                LOG("Match Found! Match is: %s", syscallMatch.toString().c_str());
                inputSeqNum = pret;
                // Get the user input from syscallMatch
                // Use ptrace to put the user input back
                writeMatchedSyscall(syscallMatch, pid);

                // Skip executing the system call
                if (skipSyscall(pid) < 0)
                {
                    LOG("Skip syscall failed: %s", syscall.toString().c_str());
                }
            }
            else
            {
                LOG("No Match Found!");
                pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                waitpid(pid, &status, 0);
            }
        }
#if 0
        else if (syscall.isOutput())
        {
            // TODO: implement later
        }
#endif
        else
        {
            //LOG("syscall nr: %lu, match found %d", regs.orig_rax, matchFound);
            //if (syscall.isValid())
            //{
                //LOG("syscall: %s", syscall.toString().c_str());
            //}

            pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            waitpid(pid, &status, 0);

            // The child process is at the point **after** a syscall.
            // Deal with the syscall here. If the match has been found previously, we shall
            // replace the return value with the recorded value in the systemcall list.
            // Most syscall will have its return value in the register %rax, But there are some
            // which does not follow the rule and we will need to deal with them seperately.
            ptrace(PTRACE_GETREGS, pid, 0, &regs);
            // We might not need the return value, but we need the side
            // effect for pid manager and fd manager.
            SystemCall syscallReturn(regs, pid, SYSARG_IFEXIT, fdManager);

            //dealWithConflict();

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
    }
    return 0;
}

int ProcessManager::skipSyscall(pid_t pid)
{
    // HW: "PTRACE_SYSEMU" is in "linux/ptrace.h" x64, but not in "sys/ptrace.h".
    // Thus I am not sure if it is supported fully by x64
    long pret;
    pret = ptrace(PTRACE_SYSEMU, pid, NULL, NULL);
    return (int) pret;
}

int ProcessManager::writeMatchedSyscall(SystemCall &syscall, pid_t pid)
{

    // TODO: Implement

    long pret;
    int ret;

    ret = syscall.overwrite(pid);
    if (ret < 0)
    {
        LOG1("Cannot put the return value into registers!");
        return ret;
    }
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
    return ss.str();
}

// The main function is used for development and debugging only.
// It will be removed in the released version
// @author haowu
#if 0
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
#endif

