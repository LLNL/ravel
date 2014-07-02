#include "p2pevent.h"

P2PEvent::P2PEvent(unsigned long long _enter, unsigned long long _exit,
                   int _function, int _process, int _phase,
                   QVector<Message *> *_messages)
    : CommEvent(_enter, _exit, _function, _process, _phase),
      subevents(NULL),
      messages(_messages),
      is_recv(false)
{
}

P2PEvent::~P2PEvent()
{
    for (QVector<Message *>::Iterator itr = messages->begin();
         itr != messages->end(); ++itr)
    {
            delete *itr;
            *itr = NULL;
    }
    delete messages;

    if (subevents)
        delete subevents;
}

P2PEvent::isReceive()
{
    return is_recv;
}


P2PEvent::set_stride_relationships(CommEvent * base)
{
    base->stride_children->insert(this);
    stride_parents->insert(base);
}
