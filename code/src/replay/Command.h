// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_COMMAND_H__
#define __REPLAY_COMMAND_H__

struct Command
{
    Vector<String> argv;
    // The pid here usually represents ``old'' pid unless ow specified.
    pid_t pid;
};

#endif //__REPLAY_COMMAND_H__

