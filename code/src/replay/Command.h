// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_COMMAND_H__
#define __REPLAY_COMMAND_H__

#include <sstream>
#include <common/common.h>

struct Command
{
    Vector<String> argv;
    // The pid here usually represents ``old'' pid unless ow specified.
    pid_t pid;

    String toString()
    {
        std::ostringstream os;
        os << pid << ":";
        for (Vector<String>::iterator it = argv.begin(); it != argv.end(); ++it)
            os << *it << " ";
        return os.str();
    }
};

#endif //__REPLAY_COMMAND_H__

