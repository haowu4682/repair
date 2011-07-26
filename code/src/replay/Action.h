// Author: Hao Wu

#ifndef __REPLAY_ACTION_H__
#define __REPLAY_ACTION_H__

#include <replay/Actor.h>

// The class is an interface.
class Action
{
    public:
        // execute the action
        virtual int exec() = 0;
        // set the actor of the action
        void setActor(Actor *actor) { this->actor = actor; }
        // get the actor
        Actor *getActor() { return actor; }
    private:
        Actor *actor;
};

#endif //__REPLAY_ACTION_H__

