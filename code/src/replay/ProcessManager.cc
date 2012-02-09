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
void *traceManagedProcess(void *managedProcess)
{
    ManagedProcess *proc = (ManagedProcess *)managedProcess;
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
    pid_t pid = fork();

    // If the fork fails
    if (pid == -1)
    {
        LOG("fork fails when trying to run %d", process.getPid());
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
    int ret = 0;

    // Let the process to be traced by parent
    long pret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    if (pret < 0)
    {
        LOG1("The child process cannot be traced!");
        return pret;
    }

    // Execute actions
    pid_t oldPid = process.getPid();
    SystemCallList *syscallList = process.getSyscallList();
    assert(syscallList != NULL);
    Vector<SystemCall> &actions = syscallList->getSystemCallListItem(oldPid).syscalls;
    LOG("Before executing actions %d", oldPid);
    for (Vector<SystemCall>::iterator it = actions.begin(); it != actions.end(); ++it)
    {
        it->exec();
    }
    LOG("After executing actions %d", oldPid);

    return ret;
}

// Deal with ``fork'' or ``vfork'' in tracing a process.
// @ret 0 when it's the manager of the original process.
//      1 when it's the manager of the new process.
//      <0 when there is an error.
int ProcessManager::dealWithFork(SystemCall &syscall)
{
    pthread_t thread;
    int ret;
    Process *childProc = process.getNextChild();
    if (childProc == NULL)
    {
        LOG("No matching child process found!");
        // TODO: report conflict
        return 0;
    }

    // update pid manager
    pid_t oldPid = childProc->getPid();
    // XXX: Hard code features for x86_64 here.
    pid_t newPid = (pid_t) syscall.getReturn();
    PidManager *pidManager = process.getPidManager();
    if (pidManager != NULL)
    {
        pidManager->add(oldPid, newPid);
    }

    ProcessManager *manager = new ProcessManager(childProc);
    ManagedProcess *managedProcess = new ManagedProcess(manager, newPid);

    // Create a pthread for a new manager
    LOG("old pid = %d, new pid = %d", oldPid, newPid);
    LOG("Before pthread create for %d in dealWithFork", oldPid);
    ret = pthread_create(&thread, NULL, traceManagedProcess, managedProcess);
    LOG("After pthread create for %d in dealWithFork", oldPid);

    // If pthread creation fails
    if (ret != 0)
    {
        LOG("pthread_create fails when trying to replay %d, errno=%d", oldPid, ret);
        return -1;
    }

    // We don't need to memorize the pid of the new manager here.
    return 0;
}

int ProcessManager::dealWithConflict(ConflictType type, const SystemCall *current,
        const SystemCall *match)
{
    char choiceChar;

    switch (type)
    {
        case differ_record:
            LOG("Type#1 Conflict Found: current: %s; record: %s",
                    current->toString().c_str(),
                    match->toString().c_str());
            cout << "A conflict is found. The record value is different from "
                "current value." << endl;
            break;
        case missing_record:
            LOG("Type#2 Conflict Found! current: %s",
                    current->toString().c_str());
            cout << "A conflict is found. The record value does not exist.";
            break;
        default:
            LOG("ERROR: Unidentified Conflict Found!");
            assert(0);
    }
    cout << "Action Needed: [M]anually resolve the conflict; [I]gnore;"
        " [P]rint more information:" << endl;

get_choice_dealing_conflict:
    choiceChar = retrieveChar("mMiIpP", "Unknown choice, try again. Action "
            "Needed: [M]anually resolve the conflict; [I]gnore; [P]rint more "
            "information:");

    switch (choiceChar)
    {
        case 'm':
        case 'M':
            // TODO: suspend running
            printMsg("System replay has currently suspended, please resolve the"
                    " issue manually, and press enter key to continue ...");
            retrieveChar();
            // TODO: resume running
            break;
        case 'i':
        case 'I':
            printMsg("Conflict is Ignored. System replay continues.");
            break;
        case 'p':
        case 'P':
            // TODO: print some information for the user
            // including: which process, what system call, which file, what
            // content, etc.
            printMsg("No more information available!");
            goto get_choice_dealing_conflict;
        default:
            LOG("ERROR: Unidentified choice");
            goto get_choice_dealing_conflict;
    }

    return 0;
}

int ProcessManager::traceProcess(pid_t pid)
{
    int status;
    int ret;
    long pret;
    // NOTE Careful of sequence number logic, it might break
    long inputSeqNum = 0;
    long selectSeqNum = 0;
    long outputSeqNum = 0;
    // Skip continueing a program at the beginning of a system call. It is used
    // if the last syscall is an execve, in which case the last system call
    // never returns.
    bool skipPtraceSyscall = false;

    struct user_regs_struct regs;
    PidManager *pidManager = process.getPidManager();
    SystemCallList *syscallList = process.getSyscallList();
    FDManager *fdManager = process.getFDManager();

    waitpid(pid, &status, 0);
    pid_t oldPid = process.getPid();

    // NOTE generation numbers might cause problems here.
    if (pidManager != NULL && oldPid != -1)
    {
        pidManager->add(oldPid, pid);
    }
    // Current the termination condition is: the child has exited from executing
    while (!WIFEXITED(status))
    {
        if (skipPtraceSyscall)
        {
            LOG("skipping syscall");
            skipPtraceSyscall = false;
            pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            waitpid(pid, &status, 0);
        }
        pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        waitpid(pid, &status, 0);

        // The child process is at the point **before** a syscall.
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        SystemCall syscallMatch;
        SystemCall syscall(regs, pid, SYSARG_IFENTER, fdManager);
        LOG("syscall found: %s", syscall.toString().c_str());

        // If the system call is user input or output, we need to act quite differently.
        if (syscall.isUserSelect(true))
        {
            pret = syscallList->searchMatch(syscallMatch, syscall, oldPid,
                    selectSeqNum, true);
            bool matchFound = (pret >= 0);

            // If a match has been found, we'll change the syscall result
            // accordingly and cancel the syscall execution. Otherwise the
            // syscall is executed as usual.
            if (matchFound)
            {
                LOG("Select Match Found! original: %s, match: %s",
                        syscall.toString().c_str(),
                        syscallMatch.toString().c_str());
                selectSeqNum = pret;

                // We do not skip executing the system call now, but these code
                // are left here in case we change our mind later.
#if 0
                timeval tmpTime;
                timeval zeroTime = {0, 0};
                long timeval_addr = SystemCall::getArgFromReg(regs, 4);
                readFromProcess(&tmpTime, timeval_addr, sizeof(zeroTime), pid);
                LOG("before time: %ld:%ld", tmpTime.tv_sec, tmpTime.tv_usec);
                writeToProcess(&zeroTime, timeval_addr, sizeof(zeroTime), pid);
                readFromProcess(&tmpTime, timeval_addr, sizeof(zeroTime), pid);
                LOG("after time: %ld:%ld", tmpTime.tv_sec, tmpTime.tv_usec);
                pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                waitpid(pid, &status, 0);

                // Achieve the result of the execution
                //ptrace(PTRACE_GETREGS, pid, 0, &regs);
                //SystemCall syscallReturn(regs, pid, SYSARG_IFEXIT, fdManager);

                // Merge the result with recorded result
                //syscallReturn.merge(syscallMatch);

                // Write matched system call
                // Get the user input from syscallMatch
                // Use ptrace to put the user input back
                //LOG("poke system call: %s", syscallReturn.toString().c_str());
                //writeMatchedSyscall(syscallReturn, pid);
#endif

                // Skip executing the system call
                if (skipSyscall(pid) < 0)
                {
                    LOG("Skip syscall failed: %s", syscall.toString().c_str());
                }

                // write back result
                writeMatchedSyscall(syscallMatch, pid);
            }
            else
            {
                LOG("No Select Match Found!");
                pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                waitpid(pid, &status, 0);
            }
        }
        else if (syscall.isRegularUserInput())
        {
            pret = syscallList->searchMatch(syscallMatch, syscall, oldPid,
                    inputSeqNum, true);
            bool matchFound = (pret >= 0);

            // If a match has been found, we'll change the syscall result
            // accordingly and cancel the syscall execution. Otherwise the
            // syscall is executed as usual.
            if (matchFound)
            {
                LOG("Input Match Found! original: %s, match: %s",
                        syscall.toString().c_str(),
                        syscallMatch.toString().c_str());
                inputSeqNum = pret;
                selectSeqNum = pret;

                // Skip executing the system call
                if (skipSyscall(pid) < 0)
                {
                    LOG("Skip syscall failed: %s", syscall.toString().c_str());
                }

                // write back result
                writeMatchedSyscall(syscallMatch, pid);
            }
            else
            {
                LOG("No Input Match Found!");
                pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                waitpid(pid, &status, 0);
            }
        }
        else if (syscall.isOutput())
        {
            pret = syscallList->searchMatch(syscallMatch, syscall, oldPid,
                    outputSeqNum, false);
            bool matchFound = (pret >= 0);

            if (!matchFound)
                // No match result has been found, report miss_record conflict
            {
                dealWithConflict(missing_record, &syscall, NULL);
            }
            else if (syscall == syscallMatch)
                // An exact match, no conflict is found
            {
//                LOG("Output Match Found! No Conflicts. record: %s",
//                        syscallMatch.toString().c_str());
                outputSeqNum = pret;
            }
            else
                // A differ_record Conflict is found
            {
                outputSeqNum = pret;
                dealWithConflict(differ_record, &syscall, &syscallMatch);
            }

            // If network output, we will skip the system call
            if (syscall.isNetworkOutput())
            {
                // Skip executing the system call
                if (skipSyscall(pid) < 0)
                {
                    LOG("Skip syscall failed: %s", syscall.toString().c_str());
                }
                // TODO: Fix this sentense with the actual value to be written
                // into the traced process here.
                writeMatchedSyscall(syscallMatch, pid);
            }
            else
            {
                pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                waitpid(pid, &status, 0);
            }
        }
        else
        {
            pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            waitpid(pid, &status, 0);

            // The child process is at the point **after** a syscall.
            // Most syscall will have its return value in the register %rax, But there are some
            // which does not follow the rule. However, they shall be handled in
            // SystemCall::init. We do not bother handling it here.
            ptrace(PTRACE_GETREGS, pid, 0, &regs);
            // We might not need the return value, but we need the side
            // effect for pid manager and fd manager.
            SystemCall syscallReturn(regs, pid, SYSARG_IFEXIT, fdManager);
            LOG("syscall return: %s", syscallReturn.toString().c_str());

            // If the system call is fork/vfork, we must create a new process manager for it.
            if (syscall.isFork())
            {
                ret = dealWithFork(syscallReturn);
                if (ret != 0)
                {
                    break;
                }
            }
            // If the system call is exec, but the return is not, we shall skip
            // one system call in the next step since there is one redundant
            // exec to be catched by ptrace.
            else if (syscall.isExec() && syscallReturn.isExec() &&
                    syscallReturn.getReturn() >= 0)
            {
                skipPtraceSyscall = true;
            }
        }
    }
    return 0;
}

int ProcessManager::skipSyscall(pid_t pid)
{
    // HW: "PTRACE_SYSEMU" is in "linux/ptrace.h" x64, but not in "sys/ptrace.h".
    // Thus we have to use an ad-hoc way here to do the same job.
    long pret;
    int status;

    user_regs_struct regs;
    pret = ptrace(PTRACE_GETREGS, pid, 0, &regs);
    if (pret < 0)
        LOG("get regs failed!");

    // XXX: ad-hoc, implicitly assume that getpid has a system call number 39
    // here.
    regs.orig_rax = 39;
    pret = ptrace(PTRACE_SETREGS, pid, 0, &regs);
    if (pret < 0)
        LOG("set regs failed!");
    pret = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
    waitpid(pid, &status, 0);
    return (int) pret;
}

int ProcessManager::writeMatchedSyscall(SystemCall &syscall, pid_t pid)
{
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
    // Currently we do not output anything.
    stringstream ss;
    return ss.str();
}

// The main function is used for development and debugging only.
// It has obsoleted, and is only reserved here in case we want to use it to
// debug or adds a mode for replaying single process
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

