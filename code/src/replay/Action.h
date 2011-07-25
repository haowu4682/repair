// Author: Hao Wu

#ifndef __REPLAY_ACTION_H__
#define __REPLAY_ACTION_H__

// The class is an interface.
class Action
{
    public:
        // exector all the actions by the actor
        virtual int exec() = 0;
};

#endif //__REPLAY_ACTION_H__

