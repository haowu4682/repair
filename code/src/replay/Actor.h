// Author: Hao Wu

#ifndef __REPLAY_ACTOR_H__
#define __REPLAY_ACTOR_H__

// The class is an interface.
class Actor
{
    public:
        // exector all the actions by the actor
        virtual int exec() = 0;
};

#endif //__REPLAY_ACTOR_H__

